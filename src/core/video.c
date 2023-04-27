/*
 * Open Surge Engine
 * video.c - video manager
 * Copyright (C) 2008-2023  Alexandre Martins <alemartf@gmail.com>
 * http://opensurge2d.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "video.h"
#include "image.h"
#include "engine.h"
#include "timer.h"
#include "logfile.h"
#include "util.h"
#include "global.h"
#include "stringutil.h"
#include "font.h"
#include "lang.h"
#include "mobile_gamepad.h"
#include "asset.h"
#include "nanoparser.h"

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_memfile.h>

#ifdef ALLEGRO_UNIX
#define ALLEGRO_UNSTABLE
#include <allegro5/allegro_x.h>
#endif


/* Display (window) */
#define DEFAULT_WINDOW_TITLE (GAME_TITLE " " GAME_VERSION_STRING)
static char window_title[256] = DEFAULT_WINDOW_TITLE;
static ALLEGRO_DISPLAY* display = NULL; /* game window */
static bool create_display();
static void destroy_display();
static void reconfigure_display();
static void compute_display_transform(ALLEGRO_TRANSFORM* transform);
static void set_display_icon(ALLEGRO_DISPLAY* display);
static void a5_handle_video_event(const ALLEGRO_EVENT* event, void* data);


/* Backbuffer */
#define USE_ROUNDROBIN_BACKBUFFER 1 /* use a small pool of FBOs in a round-robin fashion? */
#define DEFAULT_SCREEN_WIDTH  426 /* this is set on stone! Picked a 16:9 resolution */
#define DEFAULT_SCREEN_HEIGHT 240
static int game_screen_width = DEFAULT_SCREEN_WIDTH; /* the width of the backbuffer during regular gameplay (i.e., not in the level editor) */
static int game_screen_height = DEFAULT_SCREEN_HEIGHT; /* the height of the backbuffer during regular gameplay */
static image_t* backbuffer[2] = { NULL, NULL };
static int backbuffer_index = 0; /* round-robin backbuffer: 0 = primary; 1 = secondary */
static bool create_backbuffer();
static void destroy_backbuffer();
static void reconfigure_backbuffer();
static void compute_screen_size(videomode_t mode, int* screen_width, int* screen_height);


/* FPS counter */
static int fps = 0;
static int fps_counter = 0;
static double fps_time = 0.0;
static void update_fps();
static void render_fps();


/* Video settings */
static struct {

    /* current resolution */
    videoresolution_t resolution;

    /* current mode */
    videomode_t mode;

    /* fullscreen mode? */
    bool is_fullscreen;

    /* should we display the FPS counter? */
    bool is_fps_visible;

} settings = {
    .resolution = VIDEORESOLUTION_1X,
    .mode = VIDEOMODE_DEFAULT,
    .is_fullscreen = false,
    .is_fps_visible = false,
};


/* Console */
#define CONSOLE_MAX_ENTRIES       30
#define CONSOLE_MESSAGE_TIMEOUT   5.0 /* in seconds */
#define CONSOLE_MESSAGE_MAXSIZE   256
static struct {

    /* entries */
    struct {
        char message[CONSOLE_MESSAGE_MAXSIZE];
        double expire_time;
    } entry[CONSOLE_MAX_ENTRIES];

    /* index of the first empty entry */
    int head;

    /* font */
    ALLEGRO_FONT* font;

} console = {
    .head = 0,
    .font = NULL
};
static void init_console();
static void release_console();
static void print_to_console(const char* message);
static void render_console();


/* Configuration file */
static const char CONFIG_FILE[] = "surge.cfg";
static void read_config_file(const char* vpath);
static int traverse(const parsetree_statement_t* stmt);
static int traverse_game(const parsetree_statement_t* stmt);
static int traverse_video(const parsetree_statement_t* stmt);



/* Misc */
static const char LOADING_FONT[] = "Loading";
static const char LOADING_TEXT[] = "$LOADING_TEXT";
static const char LOADING_IMAGE[] = "images/loading.png";

static const char* RESOLUTION_NAME[] = {
    [VIDEORESOLUTION_1X] = "1x",
    [VIDEORESOLUTION_2X] = "2x",
    [VIDEORESOLUTION_3X] = "3x",
    [VIDEORESOLUTION_4X] = "4x"
};

static const char* VIDEOMODE_NAME[] = {
    [VIDEOMODE_DEFAULT] = "default",
    [VIDEOMODE_FILL] = "fill",
    [VIDEOMODE_BESTFIT] = "best-fit"
};

#define DRAW_TEXT(x, y, flags, fmt, ...) do { \
    al_draw_textf(console.font, al_map_rgb(0, 0, 0), (x) + 1.0f, (y) + 1.0f, (flags) | ALLEGRO_ALIGN_INTEGER, (fmt), __VA_ARGS__); \
    al_draw_textf(console.font, al_map_rgb(0, 0, 0), (x) + 0.0f, (y) + 1.0f, (flags) | ALLEGRO_ALIGN_INTEGER, (fmt), __VA_ARGS__); \
    al_draw_textf(console.font, al_map_rgb(255, 255, 255), (x), (y), (flags) | ALLEGRO_ALIGN_INTEGER, (fmt), __VA_ARGS__); \
} while(0)

#define FONT_SCALE() (( \
    al_get_display_width(display) >= 2 * game_screen_width && \
    al_get_display_height(display) >= 2 * game_screen_height \
) ? 2 : 1)

#define LOG(...)    logfile_message("Video - " __VA_ARGS__)
#define FATAL(...)  fatal_error("Video - " __VA_ARGS__)

static void render_texts();




/* -------------------- public stuff -------------------- */





/*
 * video_init()
 * Initializes the video manager
 */
void video_init()
{
    LOG("Initializing the video manager...");

    /* initialize Allegro */
    if(!al_init_image_addon())
        FATAL("Can't initialize Allegro's image addon");

    if(!al_init_primitives_addon())
        FATAL("Can't initialize Allegro's primitives addon");

    if(!al_init_font_addon()) /* initialize the font addon before creating the console font */
        FATAL("Can't initialize Allegro's font addon");

    al_inhibit_screensaver(true);

    /* load the default video settings */
    settings.mode = VIDEOMODE_DEFAULT;
    game_screen_width = DEFAULT_SCREEN_WIDTH;
    game_screen_height = DEFAULT_SCREEN_HEIGHT;
    str_cpy(window_title, DEFAULT_WINDOW_TITLE, sizeof(window_title));

    /* read the config file */
    read_config_file(CONFIG_FILE);

    /* reset the FPS counter */
    fps = 0;
    fps_counter = 0;
    fps_time = 0.0;

    /* create the display */
    if(!create_display())
        FATAL("Failed to create the display");

    /* create the backbuffer */
    if(!create_backbuffer())
        FATAL("Failed to create the backbuffer");

    /* initialize the console */
    init_console();
}

/*
 * video_release()
 * Releases the video manager
 */
void video_release()
{
    LOG("Releasing the video manager...");

    /* release the console */
    release_console();

    /* destroy the backbuffer */
    destroy_backbuffer();

    /* destroy the display */
    destroy_display();
}

/*
 * video_render()
 * Updates the video manager and the screen
 * An optional callback may be used to render an overlay in window space (not screen space)
 */
void video_render(void (*render_overlay)())
{
    ALLEGRO_TRANSFORM display_transform;
    ALLEGRO_TRANSFORM identity_transform;

    /* compute an appropriate transform */
    al_identity_transform(&identity_transform);
    compute_display_transform(&display_transform);

    /* hint the graphics driver that we no longer need the depth buffer
       just before switching the target bitmap */
    /*glInvalidateFramebuffer();*/
    al_clear_depth_buffer(1);

    /* copy our backbuffer to the display backbuffer */
    al_set_target_bitmap(al_get_backbuffer(display));
    al_use_transform(&display_transform);
#if USE_ROUNDROBIN_BACKBUFFER
#if 1
        /* render the current frame */
        al_draw_bitmap(IMAGE2BITMAP(backbuffer[backbuffer_index]), 0.0f, 0.0f, 0);
#else
        /* render the previous frame */
        al_draw_bitmap(IMAGE2BITMAP(backbuffer[1-backbuffer_index]), 0.0f, 0.0f, 0);
#endif
#else
        al_draw_bitmap(IMAGE2BITMAP(backbuffer[backbuffer_index]), 0.0f, 0.0f, 0);
#endif
    al_use_transform(&identity_transform);

    /* compute the framerate */
    update_fps();

    /* render stuff in window space */
    if(render_overlay != NULL)
        render_overlay();
    render_texts();

    /* flip display */
    al_flip_display();

    /* clearing just after flipping may provide a slight performance increase
       in some drivers (?). Allegro should call glClear() behind the scenes */
    al_clear_to_color(al_map_rgba_f(0, 0, 0, 0));
    al_clear_depth_buffer(1);

#if USE_ROUNDROBIN_BACKBUFFER
    /* use a round-robin scheme for a (possible) performance improvement,
       in an attempt to avoid pipeline stalling */
    backbuffer_index = 1 - backbuffer_index;
#endif

    /* restore our backbuffer */
    al_set_target_bitmap(IMAGE2BITMAP(backbuffer[backbuffer_index]));

    /* it's a good idea to call glClear() just after glBindFramebuffer() in
       some tiled architectures (mobile) */
    al_clear_to_color(al_map_rgba_f(0, 0, 0, 0));
    al_clear_depth_buffer(1);
}

/*
 * video_set_resolution()
 * Set the resolution, which controls the size of the window
 */
void video_set_resolution(videoresolution_t resolution)
{
    LOG("Changing the video resolution to %s", RESOLUTION_NAME[(int)resolution]);
    settings.resolution = resolution;
    reconfigure_display();
}

/*
 * video_get_resolution()
 * Get the current resolution
 */
videoresolution_t video_get_resolution()
{
    return settings.resolution;
}

/*
 * video_best_fit_resolution()
 * Compute the best fit resolution for the user
 */
videoresolution_t video_best_fit_resolution()
{
    ALLEGRO_MONITOR_INFO info;

    if(al_get_monitor_info(0, &info)) {
        int desktop_width = info.x2 - info.x1;
        int desktop_height = info.y2 - info.y1;

        if(desktop_width > 4 * game_screen_width && desktop_height > 4 * game_screen_height)
            return VIDEORESOLUTION_4X;
        else if(desktop_width > 3 * game_screen_width && desktop_height > 3 * game_screen_height)
            return VIDEORESOLUTION_3X;
        else if(desktop_width > 2 * game_screen_width && desktop_height > 2 * game_screen_height)
            return VIDEORESOLUTION_2X;
        else
            return VIDEORESOLUTION_1X;
    }

    /* if the monitor info is not available, we return a safe guess */
    return VIDEORESOLUTION_2X;
}

/*
 * video_set_fullscreen()
 * Enable/disable fullscreen
 */
void video_set_fullscreen(bool fullscreen)
{
    LOG("%s fullscreen", fullscreen ? "Enabling" : "Disabling");
    settings.is_fullscreen = fullscreen;
    reconfigure_display();
}

/*
 * video_is_fullscreen()
 * Are we in fullscreen mode?
 */
bool video_is_fullscreen()
{
    return settings.is_fullscreen;
}

/*
 * video_set_mode()
 * Set the video mode
 */
void video_set_mode(videomode_t mode)
{
    /* no need to change anything */
    if(mode == settings.mode)
        return;

    /* change the video mode */
    LOG("Setting the video mode to %s", VIDEOMODE_NAME[mode]);
    settings.mode = mode;
    reconfigure_backbuffer();
}

/*
 * video_get_mode()
 * Get the current video mode
 */
videomode_t video_get_mode()
{
    return settings.mode;
}

/*
 * video_set_fps_visible()
 * Shows/hides the FPS counter
 */
void video_set_fps_visible(bool visible)
{
    LOG("%s the FPS counter", visible ? "Enabling" : "Disabling");
    settings.is_fps_visible = visible;
}

/*
 * video_is_fps_visible()
 * Is the FPS counter visible?
 */
bool video_is_fps_visible()
{
    return settings.is_fps_visible;
}

/*
 * video_fps()
 * Get the FPS rate
 */
int video_fps()
{
    return fps;
}

/*
 * video_get_screen_size()
 * Returns the size of the backbuffer
 */
v2d_t video_get_screen_size()
{
    int screen_width = game_screen_width;
    int screen_height = game_screen_height;

    compute_screen_size(settings.mode, &screen_width, &screen_height);

    return v2d_new(screen_width, screen_height);
}

/*
 * video_get_window_size()
 * Returns the window size
 */
v2d_t video_get_window_size()
{
    if(display != NULL)
        return v2d_new(al_get_display_width(display), al_get_display_height(display));
    else
        return v2d_new(game_screen_width, game_screen_height);
}

/*
 * video_get_backbuffer()
 * Returns a pointer to the backbuffer
 */
image_t* video_get_backbuffer()
{
    return backbuffer[backbuffer_index];
}

/*
 * video_showmessage()
 * Shows a text message to the user
 */
void video_showmessage(const char *fmt, ...)
{
    char message[CONSOLE_MESSAGE_MAXSIZE];
    va_list args;

    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    print_to_console(message);
}

/*
 * video_display_loading_screen()
 * Displays a loading screen
 */
void video_display_loading_screen()
{
    image_t *img = image_load(LOADING_IMAGE);
    font_t* fnt = font_create(LOADING_FONT);
    v2d_t cam = v2d_multiply(video_get_screen_size(), 0.5f);

    image_clear(color_rgb(0, 0, 0));
    image_blit(img, 0, 0, (VIDEO_SCREEN_W - image_width(img)) / 2, (VIDEO_SCREEN_H - image_height(img)) / 2, image_width(img), image_height(img));
    font_set_align(fnt, FONTALIGN_CENTER);
    font_set_text(fnt, "%s", LOADING_TEXT);
    font_set_position(fnt, v2d_subtract(cam, v2d_new(0, font_get_textsize(fnt).y / 2)));
    font_render(fnt, cam);
    video_render(NULL);
    
    font_destroy(fnt);
}

/*
 * video_get_window_title()
 * The title of the window as a statically allocated buffer
 */
const char* video_get_window_title()
{
    return window_title;
}

/*
 * video_convert_window_to_screen()
 * Convert a pair of window to screen coordinates
 */
v2d_t video_convert_window_to_screen(v2d_t window_coordinates)
{
    ALLEGRO_TRANSFORM transform;
    v2d_t screen_coordinates = window_coordinates;

    compute_display_transform(&transform);
    al_invert_transform(&transform); /* the inverse is guaranteed to exist, since transform is a non-zero 2D scale followed by a translation */
    al_transform_coordinates(&transform, &screen_coordinates.x, &screen_coordinates.y);

    return screen_coordinates;
}


/* -------------------- private stuff -------------------- */





/*
 *
 * DISPLAY
 *
 */

/* Create the display (window) */
bool create_display()
{
    ALLEGRO_STATE state;

    /* check for duplicates */
    if(display != NULL)
        FATAL("Duplicate display");

    /* create a new display */
    al_store_state(&state, ALLEGRO_STATE_NEW_DISPLAY_PARAMETERS);
    al_set_new_display_flags(ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE | ALLEGRO_OPENGL | ALLEGRO_PROGRAMMABLE_PIPELINE);
#if defined(__ANDROID__)
    al_set_new_display_flags(al_get_new_display_flags() | ALLEGRO_OPENGL_ES_PROFILE);
#endif

    al_set_new_display_option(
        ALLEGRO_SUPPORTED_ORIENTATIONS,
        game_screen_width >= game_screen_height ?
            ALLEGRO_DISPLAY_ORIENTATION_LANDSCAPE :
            ALLEGRO_DISPLAY_ORIENTATION_PORTRAIT,
        ALLEGRO_REQUIRE
    );
    al_set_new_display_option(ALLEGRO_SUPPORT_NPOT_BITMAP, 1, ALLEGRO_SUGGEST);
    al_set_new_display_option(ALLEGRO_CAN_DRAW_INTO_BITMAP, 1, ALLEGRO_REQUIRE);
    /*al_set_new_display_option(ALLEGRO_DEPTH_SIZE, 16, ALLEGRO_SUGGEST);*/
    #if defined(ALLEGRO_VERSION_INT) && defined(AL_ID) && ALLEGRO_VERSION_INT >= AL_ID(5,2,8,0)
    al_set_new_display_option(ALLEGRO_DEFAULT_SHADER_PLATFORM, ALLEGRO_SHADER_GLSL_MINIMAL, ALLEGRO_REQUIRE); /* faster shader with no alpha testing */
    #endif

#if defined(ALLEGRO_UNIX) && !defined(ALLEGRO_RASPBERRYPI)
    set_display_icon(NULL);
#endif

#if defined(__ANDROID__)
    display = al_create_display(0, 0); /* occupy the entire screen on mobile */
#else
    display = al_create_display(game_screen_width, game_screen_height);
#endif

    al_restore_state(&state);
    if(display == NULL)
        return false;

    /* configure the display */
    al_set_window_title(display, window_title);
    al_hide_mouse_cursor(display);
    set_display_icon(display);

    /* listen to Allegro 5 events */
    engine_add_event_source(al_get_display_event_source(display));
    engine_add_event_listener(ALLEGRO_EVENT_DISPLAY_CLOSE, NULL, a5_handle_video_event);
    engine_add_event_listener(ALLEGRO_EVENT_DISPLAY_RESIZE, NULL, a5_handle_video_event);
    engine_add_event_listener(ALLEGRO_EVENT_DISPLAY_SWITCH_IN, NULL, a5_handle_video_event);
    engine_add_event_listener(ALLEGRO_EVENT_DISPLAY_SWITCH_OUT, NULL, a5_handle_video_event);
    engine_add_event_listener(ALLEGRO_EVENT_DISPLAY_HALT_DRAWING, NULL, a5_handle_video_event);
    engine_add_event_listener(ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING, NULL, a5_handle_video_event);

    /* done! */
    return true;
}

/* Destroy the display */
void destroy_display()
{
    if(display == NULL)
        FATAL("Display released twice");

    al_set_target_bitmap(NULL);
    al_destroy_display(display);
    display = NULL;
}

/* Reconfigure the display according to the current settings */
void reconfigure_display()
{
#if !defined(__ANDROID__)
    int multiplier = (int)(settings.resolution - VIDEORESOLUTION_1X) + 1;
    int new_display_width = game_screen_width * multiplier;
    int new_display_height = game_screen_height * multiplier;
    int delta_width = new_display_width - al_get_display_width(display);
    int delta_height = new_display_height - al_get_display_height(display);

    /* toggle fullscreen */
    if(!al_set_display_flag(display, ALLEGRO_FULLSCREEN_WINDOW, settings.is_fullscreen))
        LOG("Can't toggle fullscreen mode");

    /* resize the window */
    if(!(al_get_display_flags(display) & ALLEGRO_FULLSCREEN_WINDOW)) {
        if(al_resize_display(display, new_display_width, new_display_height)) {

            /* reposition the window */
            int x, y;
            al_get_window_position(display, &x, &y);

            x = max(0, x - delta_width / 2);
            y = max(0, y - delta_height / 2);
            al_set_window_position(display, x, y);

        }
        else
            LOG("Can't resize the display to %dx%d", new_display_width, new_display_height);
    }
#endif
}

/* compute a transform, so that we have a resized screen that maintains the original aspect ratio of the game */
void compute_display_transform(ALLEGRO_TRANSFORM* transform)
{
    v2d_t scale, offset;
    float display_width = (float)al_get_display_width(display);
    float display_height = (float)al_get_display_height(display);
    float backbuffer_width = (float)image_width(backbuffer[0]);
    float backbuffer_height = (float)image_height(backbuffer[0]);

    /* ensure non-zero scale for an invertible transform. is this necessary? */
    display_width = max(1, display_width);
    display_height = max(1, display_height);

    /* compute the scale */
    scale.x = display_width / backbuffer_width;
    scale.y = display_height / backbuffer_height;

    /* compute the offset */
    if(scale.x < scale.y)
        offset = v2d_new(0.0f, (display_height - scale.x * backbuffer_height) * 0.5f);
    else
        offset = v2d_new((display_width - scale.y * backbuffer_width) * 0.5f, 0.0f);

    /* compute the transform */
    al_build_transform(transform, offset.x, offset.y, min(scale.x, scale.y), min(scale.x, scale.y), 0.0f);
}

/* sets the icon of the display to a built-in icon */
void set_display_icon(ALLEGRO_DISPLAY* display)
{
    extern const unsigned char ICON_PNG[];
    extern const size_t ICON_SIZE;
    ALLEGRO_FILE* f = al_open_memfile((void*)ICON_PNG, ICON_SIZE, "rb");
    ALLEGRO_BITMAP* icon = al_load_bitmap_f(f, ".png");

    if(display != NULL)
        al_set_display_icon(display, icon);

#if defined(ALLEGRO_UNIX) && !defined(ALLEGRO_RASPBERRYPI)
    if(display == NULL)
        al_x_set_initial_icon(icon);
#endif

    al_destroy_bitmap(icon);
    al_fclose(f);
}

/* handle a video event from Allegro */
void a5_handle_video_event(const ALLEGRO_EVENT* event, void* data)
{
    switch(event->type) {

        case ALLEGRO_EVENT_DISPLAY_CLOSE:
            if(event->display.source == display)
                engine_quit();
            break;

        case ALLEGRO_EVENT_DISPLAY_RESIZE:
            al_acknowledge_resize(event->display.source);
            if(settings.mode != VIDEOMODE_DEFAULT)
                reconfigure_backbuffer();
            break;

        case ALLEGRO_EVENT_DISPLAY_SWITCH_IN:
            break;

        case ALLEGRO_EVENT_DISPLAY_SWITCH_OUT:
            break;

        case ALLEGRO_EVENT_DISPLAY_HALT_DRAWING:
            al_acknowledge_drawing_halt(event->display.source);
            destroy_backbuffer(); /* the backbuffer has the ALLEGRO_NO_PRESERVE_TEXTURE flag enabled */
            break;

        case ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING:
            al_acknowledge_drawing_resume(event->display.source);
            if(!create_backbuffer())
                FATAL("Can't create backbuffer after al_acknowledge_drawing_resume()");
            break;
    }

    (void)data;
}



/*
 *
 * BACKBUFFER
 *
 */

/* Create the backbuffer (the image texture to which the game will be rendered to) */
bool create_backbuffer()
{
    int screen_width = game_screen_width;
    int screen_height = game_screen_height;

    /* validate */
    if(backbuffer[0] != NULL) {
        FATAL("Duplicate backbuffer");
        return false;
    }

    /* compute the size of the backbuffer */
    compute_screen_size(settings.mode, &screen_width, &screen_height);

    /* create the images */
    bool want_depth_buffer = true;

    if(NULL == (backbuffer[0] = image_create_backbuffer(screen_width, screen_height, want_depth_buffer))) {
        return false;
    }
#if USE_ROUNDROBIN_BACKBUFFER
    else if(NULL == (backbuffer[1] = image_create_backbuffer(screen_width, screen_height, want_depth_buffer))) {
        image_destroy(backbuffer[0]);
        backbuffer[0] = NULL;
        return false;
    }
#endif

    /* set the target bitmap */
    backbuffer_index = 0;
    al_set_target_bitmap(IMAGE2BITMAP(backbuffer[backbuffer_index]));

    /* done! */
    return true;
}

/* Destroy the backbuffer */
void destroy_backbuffer()
{
    /* a display is tied to an OpenGL rendering context. A video bitmap is tied
       to a display. If the display is invalidated, so is the backbuffer */
    if(backbuffer[0] == NULL) {
        FATAL("Backbuffer released twice");
        return;
    }

    /* restore the default framebuffer */
    al_set_target_bitmap(al_get_backbuffer(display));

    /* destroy the images */
    for(int b = sizeof(backbuffer) / sizeof(backbuffer[0]) - 1; b >= 0; b--) {
        if(backbuffer[b] != NULL)
            image_destroy(backbuffer[b]);
        backbuffer[b] = NULL;
    }
}

/* Reconfigure the backbuffer according to the current settings */
void reconfigure_backbuffer()
{
    /* validate */
    if(backbuffer[0] == NULL) {
        FATAL("Can't reconfigure the backbuffer: no backbuffer");
        return;
    }

    /* destroy the old */
    LOG("Will reconfigure the backbuffer...");
    destroy_backbuffer();

    /* create the new */
    if(!create_backbuffer())
        FATAL("Can't reconfigure the backbuffer");
}

/* Compute the size of the screen / backbuffer according to the video mode */
void compute_screen_size(videomode_t mode, int* screen_width, int* screen_height)
{
    int window_width = al_get_display_width(display);
    int window_height = al_get_display_height(display);

    switch(mode) {
        case VIDEOMODE_DEFAULT:
            *screen_width = game_screen_width;
            *screen_height = game_screen_height;
            break;

        case VIDEOMODE_FILL:
            *screen_width = window_width;
            *screen_height = window_height;
            break;

        case VIDEOMODE_BESTFIT:
            if(game_screen_width > 0 && game_screen_height > 0) { /* do we need to do this checking? */
                float aspect_ratio = (float)game_screen_width / (float)game_screen_height;

                if(aspect_ratio >= 1.0f) {
                    *screen_width = window_width;
                    *screen_height = (int)roundf((float)window_width / aspect_ratio);
                }
                else {
                    *screen_width = (int)roundf((float)window_height * aspect_ratio);
                    *screen_height = window_height;
                }
            }
            break;
    }
}


/*
 *
 * BUILT-IN CONSOLE
 *
 */

/* Initialize the console */
void init_console()
{
    /* initialize the entries */
    for(int i = 0; i < CONSOLE_MAX_ENTRIES; i++) {
        console.entry[i].message[0] = '\0';
        console.entry[i].expire_time = 0.0;
    }

    /* initialize the head */
    console.head = 0;

    /* create a font for the console */
    console.font = al_create_builtin_font();
}

/* Release the console */
void release_console()
{
    /* release the font of the console */
    al_destroy_font(console.font);
}

/* Print a message to the console */
void print_to_console(const char* message)
{
    /* create the entry */
    str_cpy(console.entry[console.head].message, message, sizeof(console.entry[console.head].message));
    console.entry[console.head].expire_time = timer_get_elapsed() + CONSOLE_MESSAGE_TIMEOUT;

    /* advance the head */
    console.head = (console.head + 1) % CONSOLE_MAX_ENTRIES;
}

/* Render the messages of the console */
void render_console()
{
    int font_scale = FONT_SCALE();
    int font_height = al_get_font_line_height(console.font);
    int first = console.head;
    int last = (console.head + (CONSOLE_MAX_ENTRIES - 1)) % CONSOLE_MAX_ENTRIES;
    int ypos = al_get_display_height(display) / font_scale;
    double elapsed = timer_get_elapsed();

    ALLEGRO_STATE state;
    al_store_state(&state, ALLEGRO_STATE_TRANSFORM);

    ALLEGRO_TRANSFORM transform;
    al_identity_transform(&transform);
    al_scale_transform(&transform, font_scale, font_scale);
    al_use_transform(&transform);

    #define PRINT_ENTRIES(from, to) \
        for(int i = (to); i >= (from); i--) { \
            if(elapsed < console.entry[i].expire_time) { \
                ypos -= font_height; \
                DRAW_TEXT(0, ypos, ALLEGRO_ALIGN_LEFT, "%s", console.entry[i].message); \
            } \
        }

    if(first < last) {
        PRINT_ENTRIES(first, last);
    }
    else {
        PRINT_ENTRIES(0, last);
        PRINT_ENTRIES(first, CONSOLE_MAX_ENTRIES - 1);
    }

    al_restore_state(&state);
}


/*
 *
 * FRAMERATE
 *
 */

/* compute the framerate */
void update_fps()
{
    double now = timer_get_now();

    ++fps_counter;
    if(now >= fps_time + 1.0) {
        fps_time = now;
        fps = fps_counter;
        fps_counter = 0;
    }
}

/* render the FPS counter */
void render_fps()
{
    int font_scale = FONT_SCALE();
    int display_width = al_get_display_width(display);

    ALLEGRO_STATE state;
    al_store_state(&state, ALLEGRO_STATE_TRANSFORM);

    ALLEGRO_TRANSFORM transform;
    al_identity_transform(&transform);
    al_scale_transform(&transform, font_scale, font_scale);
    al_translate_transform(&transform, display_width, 0.0f);

    al_use_transform(&transform);
        DRAW_TEXT(0.0f, 0.0f, ALLEGRO_ALIGN_RIGHT, "%d", fps);
    al_restore_state(&state);
}



/*
 *
 * MISC
 *
 */

/* render texts with Allegro's built-in font */
void render_texts()
{
    al_hold_bitmap_drawing(true);

    if(settings.is_fps_visible)
        render_fps();
    render_console();

    al_hold_bitmap_drawing(false);
}



/*
 *
 * CONFIGURATION FILE
 *
 */

/* read the configuration file */
void read_config_file(const char* vpath)
{
    if(asset_exists(vpath)) {
        const char* fullpath = asset_path(vpath);
        parsetree_program_t* p = nanoparser_construct_tree(fullpath);

        nanoparser_traverse_program(p, traverse);

        nanoparser_deconstruct_tree(p);
    }

    /*

    use the default settings if the configuration file is not found
    (i.e., legacy games?)

    */
}

/* traverse the configuration file */
int traverse(const parsetree_statement_t* stmt)
{
    const char* identifier = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t* param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "game") == 0) {
        const parsetree_parameter_t* p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_program(p1, "Expected game settings");

        nanoparser_traverse_program(nanoparser_get_program(p1), traverse_game);
    }
    else if(str_icmp(identifier, "video") == 0) {
        const parsetree_parameter_t* p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_program(p1, "Expected video settings");

        nanoparser_traverse_program(nanoparser_get_program(p1), traverse_video);
    }
    else
        fatal_error("Unexpected identifier \"%s\" in %s:%d", identifier, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

    return 0;
}

/* read the game section of the configuration file */
int traverse_game(const parsetree_statement_t* stmt)
{
    const char* identifier = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t* param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "title") == 0) {
        const parsetree_parameter_t* p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "Expected game title");

        str_cpy(window_title, nanoparser_get_string(p1), sizeof(window_title));
    }
    else
        fatal_error("Unexpected identifier \"%s\" in %s:%d", identifier, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

    return 0;
}

/* read the video section of the configuration file */
int traverse_video(const parsetree_statement_t* stmt)
{
    const char* identifier = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t* param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "screen_size") == 0) {
        const parsetree_parameter_t* p1 = nanoparser_get_nth_parameter(param_list, 1);
        const parsetree_parameter_t* p2 = nanoparser_get_nth_parameter(param_list, 2);
        nanoparser_expect_string(p1, "Expected screen_size: width, height");
        nanoparser_expect_string(p2, "Expected screen_size: width, height");

        game_screen_width = atoi(nanoparser_get_string(p1));
        game_screen_height = atoi(nanoparser_get_string(p2));

        if(game_screen_width <= 0 || game_screen_height <= 0)
            fatal_error("Invalid screen_size (%d x %d) in %s:%d", game_screen_width, game_screen_height, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));
    }
    else
        fatal_error("Unexpected identifier \"%s\" in %s:%d", identifier, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

    return 0;
}