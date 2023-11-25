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

#define ALLEGRO_UNSTABLE /* al_x_set_initial_icon(), al_clear_keyboard_state() */
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_memfile.h>
#include <allegro5/allegro_opengl.h>

#ifdef ALLEGRO_UNIX
#include <allegro5/allegro_x.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "video.h"
#include "image.h"
#include "shader.h"
#include "engine.h"
#include "timer.h"
#include "logfile.h"
#include "global.h"
#include "font.h"
#include "lang.h"
#include "asset.h"
#include "nanoparser.h"
#include "../util/util.h"
#include "../util/stringutil.h"
#include "../entities/mobilegamepad.h"



/* Display (window) */
#define DEFAULT_WINDOW_TITLE (GAME_TITLE " " GAME_VERSION_STRING)
static char window_title[256] = DEFAULT_WINDOW_TITLE;
static ALLEGRO_DISPLAY* display = NULL; /* game window */
static bool create_display(int width, int height);
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


/* OpenGL-specific */
ALLEGRO_DEFINE_PROC_TYPE(void, fun_glinvalidateframebuffer_t, (GLenum, GLsizei, const GLenum*));
ALLEGRO_DEFINE_PROC_TYPE(void, fun_glclear_t, (GLbitfield));
ALLEGRO_DEFINE_PROC_TYPE(void, fun_glclearcolor_t, (GLfloat, GLfloat, GLfloat, GLfloat));
ALLEGRO_DEFINE_PROC_TYPE(void, fun_glcleardepth_t, (GLdouble));
static fun_glinvalidateframebuffer_t _glInvalidateFramebuffer = NULL;
static fun_glclear_t _glClear = NULL;
static fun_glclearcolor_t _glClearColor = NULL;
static fun_glcleardepth_t _glClearDepth = NULL;
static void import_opengl_symbols();


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

    /* current quality */
    videoquality_t quality;

    /* fullscreen mode? */
    bool is_fullscreen;

    /* immersive mode? */
    bool is_immersive;

    /* should we display the FPS counter? */
    bool is_fps_visible;

} settings = {
    .resolution = VIDEORESOLUTION_1X,
    .mode = VIDEOMODE_DEFAULT,
    .quality = VIDEOQUALITY_MEDIUM,
    .is_fullscreen = false,
    .is_immersive = false,
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


/* Loading screen */
static const char LOADING_FONT[] = "Loading";
static const char LOADING_TEXT[] = "$LOADING_TEXT";
static const char LOADING_IMAGE[] = "images/loading.png";


/* Misc */
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

static const char* VIDEOQUALITY_NAME[] = {
    [VIDEOQUALITY_LOW] = "low",
    [VIDEOQUALITY_MEDIUM] = "medium",
    [VIDEOQUALITY_HIGH] = "high"
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
static bool use_default_shader();






/* -------------------- public stuff -------------------- */





/*
 * video_init()
 * Initializes the video manager
 */
void video_init()
{
    LOG("Initializing the video manager...");

    /* initialize Allegro */
    if(!al_is_image_addon_initialized()) {
        if(!al_init_image_addon())
            FATAL("Can't initialize Allegro's image addon");
    }

    if(!al_is_primitives_addon_initialized()) {
        if(!al_init_primitives_addon())
            FATAL("Can't initialize Allegro's primitives addon");
    }

    if(!al_is_font_addon_initialized()) {
        if(!al_init_font_addon()) /* initialize the font addon before creating the console font */
            FATAL("Can't initialize Allegro's font addon");
    }

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
    if(!create_display(game_screen_width, game_screen_height))
        FATAL("Failed to create a %dx%d display", game_screen_width, game_screen_height);

    /* create the backbuffer */
    if(!create_backbuffer())
        FATAL("Failed to create the backbuffer");

    /* import OpenGL symbols */
    import_opengl_symbols();

    /* initialize the shader system */
    shader_init();
    if(!use_default_shader())
        FATAL("Failed to use the default shader");

    /* initialize the console */
    init_console();

    /* initialize in immersive mode */
    video_set_immersive(true);
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

    /* release the shader system */
    shader_release();

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
    if(_glInvalidateFramebuffer != NULL) {
        static const GLenum attachments[1] = { GL_DEPTH_ATTACHMENT };
        _glInvalidateFramebuffer(GL_FRAMEBUFFER, 1, attachments);
    }
    else
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

    /* OpenGL: clear values */
    if(_glClearColor != NULL)
        _glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    if(_glClearDepth != NULL)
        _glClearDepth(1.0);

    /* clearing just after flipping may provide a slight performance increase
       in some drivers */
    if(_glClear != NULL)
        _glClear(GL_COLOR_BUFFER_BIT);
    else
        al_clear_to_color(al_map_rgba_f(0.0f, 0.0f, 0.0f, 0.0f));

#if USE_ROUNDROBIN_BACKBUFFER
    /* use a round-robin scheme for a (possible) performance improvement,
       in an attempt to avoid pipeline stalling */
    backbuffer_index = 1 - backbuffer_index;
#endif

    /* restore our backbuffer */
    al_set_target_bitmap(IMAGE2BITMAP(backbuffer[backbuffer_index]));

    /* it's a good idea to call glClear() just after glBindFramebuffer() in
       some tiled architectures (mobile) */
    if(_glClear != NULL) {
        /* clear color & depth buffers in a single call */
        _glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    else {
        /* Allegro should call glClear() behind the scenes */
        al_clear_to_color(al_map_rgba_f(0.0f, 0.0f, 0.0f, 0.0f));
        al_clear_depth_buffer(1);
    }

    /*

    See also:
    https://community.arm.com/arm-community-blogs/b/graphics-gaming-and-vr-blog/posts/mali-performance-2-how-to-correctly-handle-framebuffers

    */
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
 * video_set_immersive()
 * Enable/disable immersive mode
 */
void video_set_immersive(bool immersive)
{
    bool changed_mode = (settings.is_immersive != immersive);

    /* log */
    if(changed_mode)
        LOG("%s immersive mode", immersive ? "Enabling" : "Disabling");

#if defined(__ANDROID__)
    /* Android-only */
    al_set_display_flag(display, ALLEGRO_FRAMELESS, immersive);

    /* restore the default shader */
    if(changed_mode) {
        if(!use_default_shader())
            LOG("Can't set the default shader");
    }
#endif

    /* update the internal flag */
    settings.is_immersive = immersive;
}

/*
 * video_is_immersive()
 * Are we in immersive mode?
 */
bool video_is_immersive()
{
    return settings.is_immersive;
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
 * video_set_quality()
 * Set the video quality
 */
void video_set_quality(videoquality_t quality)
{
    /* no need to change anything */
    if(quality == settings.quality)
        return;

    /* change the video quality */
    LOG("Setting the video quality to %s", VIDEOQUALITY_NAME[quality]);
    settings.quality = quality;

    /* TODO */
    if(quality > VIDEOQUALITY_MEDIUM)
        video_showmessage("%s video quality: coming soon!", VIDEOQUALITY_NAME[quality]);
}

/*
 * video_get_quality()
 * Get the current video quality
 */
videoquality_t video_get_quality()
{
    return settings.quality;
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
 * Displays a text message in the built-in console
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
 * video_clearmessages()
 * Clears the built-in console
 */
void video_clearmessages()
{
    for(int i = 0; i < CONSOLE_MAX_ENTRIES; i++)
        print_to_console("");
}

/*
 * video_display_loading_screen()
 * Displays a loading screen
 */
void video_display_loading_screen()
{
    const image_t *img = image_load(LOADING_IMAGE);
    v2d_t camera = v2d_multiply(video_get_screen_size(), 0.5f);

    /* prepare the font */
    font_t* fnt = font_create(LOADING_FONT);
    font_set_align(fnt, FONTALIGN_CENTER);
    font_set_text(fnt, "%s", LOADING_TEXT);
    font_set_position(fnt, v2d_subtract(camera, v2d_new(0, font_get_textsize(fnt).y / 2)));

    /* make sure we're using the default shader */
    use_default_shader();

    /* render */
    image_clear(color_rgb(0, 0, 0));
    image_draw(img, (VIDEO_SCREEN_W - image_width(img)) / 2, (VIDEO_SCREEN_H - image_height(img)) / 2, IF_NONE);
    font_render(fnt, camera);
    video_render(NULL);

    /* cleanup */
    font_destroy(fnt);
    image_unload(img);
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

/*
 * video_take_snapshot()
 * Take a screenshot (a copy of the backbuffer)
 * Destroy the returned image after usage
 */
image_t* video_take_snapshot()
{
#if USE_ROUNDROBIN_BACKBUFFER
    int index = 1 - backbuffer_index;
#else
    int index = backbuffer_index;
#endif
    const image_t* snapshot = backbuffer[index];
    return image_clone(snapshot);
}

/*
 * video_use_default_shader()
 * Use the default shader. THIS IS NOT MEANT TO BE USED IN A LOOP.
 * Returns true on success
 */
bool video_use_default_shader()
{
    return use_default_shader();
}



/* -------------------- private stuff -------------------- */





/*
 *
 * DISPLAY
 *
 */

/* Create the display (window) */
bool create_display(int width, int height)
{
    ALLEGRO_STATE state;

    LOG("Creating the display...");

    /* check for duplicates */
    if(display != NULL)
        FATAL("Duplicate display");

    /* create a new display */
    al_store_state(&state, ALLEGRO_STATE_NEW_DISPLAY_PARAMETERS);
    al_set_new_display_flags(
        ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE |
        ALLEGRO_OPENGL | ALLEGRO_PROGRAMMABLE_PIPELINE
    );

#if defined(__ANDROID__)
    /* require OpenGL ES 3.0+ on mobile */
    al_set_new_display_flags(al_get_new_display_flags() | ALLEGRO_OPENGL_ES_PROFILE);
    al_set_new_display_option(ALLEGRO_OPENGL_MAJOR_VERSION, 3, ALLEGRO_REQUIRE);
    al_set_new_display_option(ALLEGRO_OPENGL_MINOR_VERSION, 0, ALLEGRO_REQUIRE);
#elif 0
    /* request OpenGL 3.3+ on Desktop */
    /* does not work properly, why? */
    al_set_new_display_flags(al_get_new_display_flags() | ALLEGRO_OPENGL_3_0);
    al_set_new_display_flags(al_get_new_display_flags() | ALLEGRO_OPENGL_CORE_PROFILE);
    al_set_new_display_option(ALLEGRO_OPENGL_MAJOR_VERSION, 3, ALLEGRO_SUGGEST);
    al_set_new_display_option(ALLEGRO_OPENGL_MINOR_VERSION, 3, ALLEGRO_SUGGEST);
#else
    /* create an OpenGL context with "default" settings. Will likely work.
       (we should require 3.3+ instead, or ES 3.0+) */
    ;
#endif

#if 0
    /* specifying these may not work as expected? should be a given in newer OpenGL versions */
    al_set_new_display_option(ALLEGRO_SUPPORT_NPOT_BITMAP, 1, ALLEGRO_SUGGEST);
    al_set_new_display_option(ALLEGRO_CAN_DRAW_INTO_BITMAP, 1, ALLEGRO_REQUIRE);
    /*al_set_new_display_option(ALLEGRO_DEPTH_SIZE, 16, ALLEGRO_SUGGEST);*/
#endif
    al_set_new_display_option(ALLEGRO_COLOR_SIZE, 32, ALLEGRO_SUGGEST);

    if(game_screen_width >= game_screen_height)
        al_set_new_display_option(ALLEGRO_SUPPORTED_ORIENTATIONS, ALLEGRO_DISPLAY_ORIENTATION_LANDSCAPE, ALLEGRO_SUGGEST);
    else
        al_set_new_display_option(ALLEGRO_SUPPORTED_ORIENTATIONS, ALLEGRO_DISPLAY_ORIENTATION_PORTRAIT, ALLEGRO_SUGGEST);

#if defined(ALLEGRO_VERSION_INT) && defined(AL_ID) && ALLEGRO_VERSION_INT >= AL_ID(5,2,8,0)
    al_set_new_display_option(ALLEGRO_DEFAULT_SHADER_PLATFORM, ALLEGRO_SHADER_GLSL_MINIMAL, ALLEGRO_REQUIRE); /* faster shader with no alpha testing */
#endif

#if defined(ALLEGRO_UNIX) && !defined(ALLEGRO_RASPBERRYPI)
    set_display_icon(NULL);
#endif

#if defined(__ANDROID__)
    display = al_create_display(0, 0); /* occupy the entire screen on mobile */
#else
    display = al_create_display(width, height);
#endif

    al_restore_state(&state);
    if(display == NULL)
        return false;

    /* the display was created. Log OpenGL version & variant */
    const char* opengl_variant = (al_get_opengl_variant() == ALLEGRO_OPENGL_ES) ? "OpenGL ES" : "OpenGL";
    LOG("We're using %s version 0x%08x", opengl_variant, al_get_opengl_version());

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
    LOG("Destroying the display...");

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

            /* restore the default shader */
            if(!use_default_shader())
                LOG("Can't set the default shader");

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
    float s = min(scale.x, scale.y);
    al_build_transform(transform, offset.x, offset.y, s, s, 0.0f);
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
    static bool was_immersive = false;

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
            al_clear_keyboard_state(event->display.source);
            break;

        case ALLEGRO_EVENT_DISPLAY_HALT_DRAWING:
            al_acknowledge_drawing_halt(event->display.source);
            destroy_backbuffer(); /* the backbuffer has the ALLEGRO_NO_PRESERVE_TEXTURE flag enabled */
            shader_discard_all();
            was_immersive = video_is_immersive();
            break;

        case ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING:
            al_acknowledge_drawing_resume(event->display.source);

            if(!create_backbuffer())
                FATAL("Can't create backbuffer after al_acknowledge_drawing_resume()");

            shader_recreate_all();
            if(!use_default_shader())
                LOG("Can't set the default shader");

            video_set_immersive(was_immersive);
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

    /* reset shader */
    if(!use_default_shader())
        LOG("Can't set the default shader");
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
 * DEFAULT SHADER
 *
 */

/* use the default shader */
bool use_default_shader()
{
    const shader_t* default_shader = shader_get_default();

#if USE_ROUNDROBIN_BACKBUFFER
    if(backbuffer[0] == NULL || backbuffer[1] == NULL)
        return shader_set_active(default_shader);

    /* According to the Allegro manual, al_use_shader() "uses the shader for
       subsequent drawing operations on the current target bitmap".

       https://liballeg.org/a5docs/trunk/shader.html */

    /* *** THIS IS SLOW ***
       not meant to be used in a loop! */
    al_set_target_bitmap(IMAGE2BITMAP(backbuffer[backbuffer_index]));
    bool a = shader_set_active(default_shader);
    al_set_target_bitmap(IMAGE2BITMAP(backbuffer[1 - backbuffer_index]));
    bool b = shader_set_active(default_shader);
    al_set_target_bitmap(IMAGE2BITMAP(backbuffer[backbuffer_index]));

    return a && b;
#else
    return shader_set_active(default_shader);
#endif
}



/*
 *
 * BUILT-IN CONSOLE
 *
 */

/* Initialize the console */
void init_console()
{
    LOG("Initializing the console...");

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
    LOG("Releasing the console...");

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
#if !defined(__ANDROID__)
    int font_scale = FONT_SCALE();
    int display_width = al_get_display_width(display);
    int xpos = display_width;
#else
    int font_scale = 2;
    int display_width = al_get_display_width(display);
    int display_height = al_get_display_height(display);
    int backbuffer_width = image_width(backbuffer[0]);
    int backbuffer_height = image_height(backbuffer[0]);
    float scale = (float)display_height / (float)backbuffer_height;

    int xpos = (display_width + scale * backbuffer_width) * 0.5f;
    if(xpos > display_width)
        xpos = display_width;
#endif

    ALLEGRO_STATE state;
    al_store_state(&state, ALLEGRO_STATE_TRANSFORM);

    ALLEGRO_TRANSFORM transform;
    al_identity_transform(&transform);
    al_scale_transform(&transform, font_scale, font_scale);
    al_translate_transform(&transform, xpos, 0.0f);

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

/* import OpenGL symbols */
void import_opengl_symbols()
{
#if 1
    /* glClear & friends */
    _glClear = (fun_glclear_t)al_get_opengl_proc_address("glClear");
    _glClearColor = (fun_glclearcolor_t)al_get_opengl_proc_address("glClearColor");
    _glClearDepth = (fun_glcleardepth_t)al_get_opengl_proc_address("glClearDepth");

    /* glInvalidateFramebuffer: OpenGL 4.3+ and OpenGL ES 3.0+ */
    _glInvalidateFramebuffer = (fun_glinvalidateframebuffer_t)al_get_opengl_proc_address("glInvalidateFramebuffer");
#else
    _glClear = NULL;
    _glClearColor = NULL;
    _glClearDepth = NULL;
    _glInvalidateFramebuffer = NULL;
#endif

    /* log */
    #define LOG_GL(symbol) LOG("Found gl" #symbol ": %s", (_gl ## symbol != NULL) ? "yes" : "no")

    LOG("Importing OpenGL symbols");
    LOG_GL(Clear);
    LOG_GL(ClearColor);
    LOG_GL(ClearDepth);
    LOG_GL(InvalidateFramebuffer);

    #undef LOG_GL
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
