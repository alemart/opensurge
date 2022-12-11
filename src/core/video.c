/*
 * Open Surge Engine
 * video.c - video manager
 * Copyright (C) 2008-2022  Alexandre Martins <alemartf@gmail.com>
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


/* Backbuffer */
#define DEFAULT_SCREEN_WIDTH  426 /* this is set on stone! Picked a 16:9 resolution */
#define DEFAULT_SCREEN_HEIGHT 240
static int game_screen_width = DEFAULT_SCREEN_WIDTH; /* the width of the backbuffer during regular gameplay (i.e., not in the level editor) */
static int game_screen_height = DEFAULT_SCREEN_HEIGHT; /* the height of the backbuffer during regular gameplay */
static image_t* backbuffer = NULL;
static bool create_backbuffer();
static void destroy_backbuffer();
static void reconfigure_backbuffer();


/* FPS counter */
static int fps = 0;
static int fps_counter = 0;
static double fps_time = 0.0;


/* Video settings */
static struct {

    /* current resolution */
    videoresolution_t resolution;

    /* fullscreen mode? */
    bool is_fullscreen;

    /* should we display the FPS counter? */
    bool is_fps_visible;

    /* should the size of the backbuffer match the size of the screen or of the window? */
    bool is_in_game_mode;

} settings = {
    .resolution = VIDEORESOLUTION_1X,
    .is_fullscreen = false,
    .is_fps_visible = false,
    .is_in_game_mode = true,
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


/* Misc */
static const char LOADING_FONT[] = "Loading";
static const char LOADING_TEXT[] = "LOADING_TEXT";
static const char LOADING_IMAGE[] = "images/loading.png";

static const char* RESOLUTION_NAME[] = {
    [VIDEORESOLUTION_1X] = "1x",
    [VIDEORESOLUTION_2X] = "2x",
    [VIDEORESOLUTION_3X] = "3x",
    [VIDEORESOLUTION_4X] = "4x"
};

#define DRAW_TEXT(x, y, flags, fmt, ...) do { \
    al_draw_textf(console.font, al_map_rgb(0, 0, 0), (x) + 1.0f, (y) + 1.0f, (flags) | ALLEGRO_ALIGN_INTEGER, (fmt), __VA_ARGS__); \
    al_draw_textf(console.font, al_map_rgb(0, 0, 0), (x) + 0.0f, (y) + 1.0f, (flags) | ALLEGRO_ALIGN_INTEGER, (fmt), __VA_ARGS__); \
    al_draw_textf(console.font, al_map_rgb(255, 255, 255), (x), (y), (flags) | ALLEGRO_ALIGN_INTEGER, (fmt), __VA_ARGS__); \
} while(0)





/* -------------------- public stuff -------------------- */





/*
 * video_init()
 * Initializes the video manager
 */
void video_init()
{
    logfile_message("Initializing the video manager...");

    /* initialize Allegro */
    if(!al_init_image_addon())
        fatal_error("Can't initialize Allegro's image addon");

    if(!al_init_primitives_addon())
        fatal_error("Can't initialize Allegro's primitives addon");

    if(!al_init_font_addon()) /* initialize the font addon before creating the console font */
        fatal_error("Can't initialize Allegro's font addon");

    al_inhibit_screensaver(true);

    /* load the default video settings */
    game_screen_width = DEFAULT_SCREEN_WIDTH;
    game_screen_height = DEFAULT_SCREEN_HEIGHT;
    str_cpy(window_title, DEFAULT_WINDOW_TITLE, sizeof(window_title));

    /* reset the FPS counter */
    fps = 0;
    fps_counter = 0;
    fps_time = 0.0;

    /* create the display */
    if(!create_display())
        fatal_error("Failed to create the display");

    /* create the backbuffer */
    if(!create_backbuffer())
        fatal_error("Failed to create the backbuffer");

    /* initialize the console */
    init_console();
}

/*
 * video_release()
 * Releases the video manager
 */
void video_release()
{
    logfile_message("Releasing the video manager...");

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
 */
void video_render()
{
    ALLEGRO_STATE state;
    ALLEGRO_TRANSFORM display_transform;
    double now = timer_get_now();

    /* compute the framerate */
    ++fps_counter;
    if(now >= fps_time + 1.0) {
        fps_time = now;
        fps = fps_counter;
        fps_counter = 0;
    }

    /* display the framerate */
    if(settings.is_fps_visible) {
        DRAW_TEXT(image_width(backbuffer), 0.0f, ALLEGRO_ALIGN_RIGHT, "%d", fps);
    }

    /* copy the backbuffer with an appropriate transform and render the console */
    al_store_state(&state, ALLEGRO_STATE_TRANSFORM | ALLEGRO_STATE_TARGET_BITMAP);
    al_set_target_bitmap(al_get_backbuffer(display));
    al_clear_to_color(al_map_rgb(0, 0, 0));

    compute_display_transform(&display_transform);
    al_use_transform(&display_transform);
    al_draw_bitmap(IMAGE2BITMAP(backbuffer), 0.0f, 0.0f, 0);
    render_console();

    al_restore_state(&state);
    al_flip_display();
}

/*
 * video_set_resolution()
 * Set the resolution, which controls the size of the window
 */
void video_set_resolution(videoresolution_t resolution)
{
    logfile_message("Changing the video resolution to %s", RESOLUTION_NAME[(int)resolution]);
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
    logfile_message("%s fullscreen", fullscreen ? "Enabling" : "Disabling");
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
 * video_set_game_mode()
 * Enable/disable the game mode
 */
void video_set_game_mode(bool in_game_mode)
{
    logfile_message("%s the game mode", in_game_mode ? "Enabling" : "Disabling");
    settings.is_in_game_mode = in_game_mode;
    reconfigure_backbuffer();
}

/*
 * video_is_in_game_mode()
 * Is the game mode enabled?
 */
bool video_is_in_game_mode()
{
    return settings.is_in_game_mode;
}

/*
 * video_set_fps_visible()
 * Shows/hides the FPS counter
 */
void video_set_fps_visible(bool visible)
{
    logfile_message("%s the FPS counter", visible ? "Enabling" : "Disabling");
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
    if(!settings.is_in_game_mode && backbuffer != NULL)
        return v2d_new(image_width(backbuffer), image_height(backbuffer));
    else
        return v2d_new(game_screen_width, game_screen_height);
}

/*
 * video_get_window_size()
 * Returns the window size, based on the current resolution
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
    return backbuffer;
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
    font_set_text(fnt, "%s", lang_get(LOADING_TEXT));
    font_set_position(fnt, v2d_subtract(cam, v2d_new(0, font_get_textsize(fnt).y / 2)));
    font_render(fnt, cam);
    video_render();
    
    font_destroy(fnt);
}


/*
 * a5_handle_video_event()
 * Handle a video event from Allegro
 */
void a5_handle_video_event(const ALLEGRO_EVENT* event)
{
    switch(event->type) {

        case ALLEGRO_EVENT_DISPLAY_CLOSE:
            if(event->display.source == display)
                engine_quit();
            break;

        case ALLEGRO_EVENT_DISPLAY_RESIZE:
            al_acknowledge_resize(event->display.source);
            break;

        case ALLEGRO_EVENT_DISPLAY_SWITCH_IN:
            break;

        case ALLEGRO_EVENT_DISPLAY_SWITCH_OUT:
            break;

        case ALLEGRO_EVENT_DISPLAY_HALT_DRAWING:
            al_acknowledge_drawing_halt(event->display.source);
            break;

        case ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING:
            al_acknowledge_drawing_resume(event->display.source);
            reconfigure_backbuffer(); /* the backbuffer has the ALLEGRO_NO_PRESERVE_TEXTURE flag enabled */
            break;

    }
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
    extern ALLEGRO_EVENT_QUEUE* a5_event_queue;
    ALLEGRO_STATE state;

    if(display != NULL)
        fatal_error("Duplicate display");

    al_store_state(&state, ALLEGRO_STATE_NEW_DISPLAY_PARAMETERS);
    al_set_new_display_flags(ALLEGRO_WINDOWED | ALLEGRO_OPENGL | ALLEGRO_PROGRAMMABLE_PIPELINE);
    al_set_new_display_option(
        ALLEGRO_SUPPORTED_ORIENTATIONS,
        game_screen_width >= game_screen_height ?
            ALLEGRO_DISPLAY_ORIENTATION_LANDSCAPE :
            ALLEGRO_DISPLAY_ORIENTATION_PORTRAIT,
        ALLEGRO_REQUIRE
    );

#if defined(ALLEGRO_UNIX) && !defined(ALLEGRO_RASPBERRYPI)
    set_display_icon(NULL);
#endif

#if defined(__ANDROID__)
    display = al_create_display(0, 0); /* occupy the entire screen on mobile */
#else
    display = al_create_display(game_screen_width, game_screen_height);
#endif

    if(display == NULL)
        return false;

    al_register_event_source(a5_event_queue, al_get_display_event_source(display));
    al_set_window_title(display, window_title);
    al_hide_mouse_cursor(display);
    set_display_icon(display);

    al_restore_state(&state);
    return true;
}

/* Destroy the display */
void destroy_display()
{
    if(display == NULL)
        fatal_error("Display released twice");

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
        logfile_message("VIDEO: can't toggle fullscreen mode");

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
            logfile_message("VIDEO: can't resize the display to %dx%d", new_display_width, new_display_height);
    }
#endif
}

/* compute a transform, so that we have a resized screen that maintains the original aspect ratio of the game */
void compute_display_transform(ALLEGRO_TRANSFORM* transform)
{
    v2d_t scale, offset;

    /* compute the scale */
    scale.x = (float)al_get_display_width(display) / (float)image_width(backbuffer);
    scale.y = (float)al_get_display_height(display) / (float)image_height(backbuffer);

    /* compute the offset */
    if(scale.x < scale.y)
        offset = v2d_new(0.0f, (al_get_display_height(display) - scale.x * image_height(backbuffer)) * 0.5f);
    else
        offset = v2d_new((al_get_display_width(display) - scale.y * image_width(backbuffer)) * 0.5f, 0.0f);

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



/*
 *
 * BACKBUFFER
 *
 */

/* Create the backbuffer (the image texture to which the game will be rendered to) */
bool create_backbuffer()
{
    ALLEGRO_STATE state;

    if(backbuffer != NULL)
        fatal_error("Duplicate backbuffer");

    al_store_state(&state, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);
    al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP | ALLEGRO_NO_PRESERVE_TEXTURE);

    if(NULL == (backbuffer = image_create(game_screen_width, game_screen_height)))
        return false;

    al_restore_state(&state);
    al_set_target_bitmap(IMAGE2BITMAP(backbuffer));
    return true;
}

/* Destroy the backbuffer */
void destroy_backbuffer()
{
    /* a display is tied to an OpenGL rendering context. A video bitmap is tied
       to a display. If the display is invalidated, so is the backbuffer */
    if(backbuffer == NULL)
        fatal_error("Backbuffer released twice");

    al_set_target_bitmap(al_get_backbuffer(display));
    image_destroy(backbuffer);
    backbuffer = NULL;
}

/* Reconfigure the backbuffer according to the current settings */
void reconfigure_backbuffer()
{
    ALLEGRO_STATE state;
    int backbuffer_width = settings.is_in_game_mode ? game_screen_width : al_get_display_width(display);
    int backbuffer_height = settings.is_in_game_mode ? game_screen_height : al_get_display_height(display);

    if(backbuffer == NULL)
        fatal_error("No backbuffer");
    else
        image_destroy(backbuffer);

    al_store_state(&state, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);
    al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP | ALLEGRO_NO_PRESERVE_TEXTURE);

    if(NULL == (backbuffer = image_create(backbuffer_width, backbuffer_height)))
        fatal_error("Can't reconfigure the backbuffer");

    al_restore_state(&state);
    al_set_target_bitmap(IMAGE2BITMAP(backbuffer));

    /* it seems that we need this for some reason on Android. Why? */
    al_destroy_font(console.font);
    console.font = al_create_builtin_font();
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
    int font_scale = (settings.resolution == VIDEORESOLUTION_1X) ? 1 : 2;
    int font_height = al_get_font_line_height(console.font);
    int first = console.head;
    int last = (console.head + (CONSOLE_MAX_ENTRIES - 1)) % CONSOLE_MAX_ENTRIES;
    int ypos = al_get_display_height(display) / font_scale;
    double elapsed = timer_get_elapsed();

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
}
