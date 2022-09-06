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

#define PRINT(x, y, flags, fmt, ...) do { \
    al_draw_textf(font, al_map_rgb(0, 0, 0), (x) + 1.0f, (y) + 1.0f, (flags) | ALLEGRO_ALIGN_INTEGER, (fmt), __VA_ARGS__); \
    al_draw_textf(font, al_map_rgb(0, 0, 0), (x) + 0.0f, (y) + 1.0f, (flags) | ALLEGRO_ALIGN_INTEGER, (fmt), __VA_ARGS__); \
    al_draw_textf(font, al_map_rgb(255, 255, 255), (x), (y), (flags) | ALLEGRO_ALIGN_INTEGER, (fmt), __VA_ARGS__); \
} while(0)

static ALLEGRO_DISPLAY* display = NULL;
static image_t* backbuffer = NULL;
static ALLEGRO_FONT* font = NULL;
static int suggested_bpp = 32;
static void apply_display_transform(ALLEGRO_DISPLAY* display, image_t* backbuffer);
static void set_display_icon(ALLEGRO_DISPLAY* display);

/* private stuff */
#define DEFAULT_SCREEN_SIZE     (v2d_t){ 426, 240 }    /* this is set on stone! Picked a 16:9 resolution */
static const char WINDOW_TITLE[] = GAME_TITLE " " GAME_VERSION_STRING;

/* video manager */
static v2d_t screen_size = DEFAULT_SCREEN_SIZE; /* represents the size of the screen. This may change (eg, is the user on the level editor?) */
static videoresolution_t video_resolution = VIDEORESOLUTION_1X;
static bool video_fullscreen = false;
static bool video_showfps = false;
static bool video_smooth = false;
static int fps_rate = 0;

/* Video Message */
static const uint32_t VIDEOMSG_TIMEOUT = 5000; /* milliseconds */
static const int VIDEOMSG_MAXLINES = 30;

typedef struct videomsg_t {
    char* message;
    uint32_t endtime;
    struct videomsg_t* next;
} videomsg_t;

static videomsg_t* videomsg_new(const char* message, videomsg_t* next);
static videomsg_t* videomsg_delete(videomsg_t* videomsg);
static videomsg_t* videomsg_render(videomsg_t* videomsg, int line);
static videomsg_t* videomsg = NULL;



/* video manager */

/*
 * video_init()
 * Initializes the video manager
 */
void video_init(videoresolution_t resolution, bool smooth, bool fullscreen, int bpp)
{
    logfile_message("Initializing the video...");

    /* initializing Allegro */
    if(!al_init_image_addon())
        fatal_error("Can't initialize Allegro's image addon");

    if(!al_init_primitives_addon())
        fatal_error("Can't initialize Allegro's primitives addon");

    if(!al_init_font_addon()) /* we need this here; see 'font' below */
        fatal_error("Can't initialize Allegro's font addon");

    /* setup color depth */
    if(!(bpp == 16 || bpp == 24 || bpp == 32))
        fatal_error("Invalid color depth: %d. Valid modes are: 16, 24, 32.", bpp);
    suggested_bpp = bpp;

    /* video init */
    display = NULL;
    backbuffer = NULL;
    video_changemode(resolution, smooth, fullscreen);

    /* console font */
    font = al_create_builtin_font();

    /* video message */
    videomsg = NULL;

    /* set window icon */
    set_display_icon(display);
}

/*
 * video_changemode()
 * Sets up the game window
 */
void video_changemode(videoresolution_t resolution, bool smooth, bool fullscreen)
{
    extern ALLEGRO_EVENT_QUEUE* a5_event_queue;
    bool prev_fullscreen = video_fullscreen;
    
    /* Change the video mode */
    video_resolution = resolution;
    video_fullscreen = fullscreen;
    video_smooth = false; /* not supported yet */

    /* Log the event */
    logfile_message(
        "Changing the video mode to 0x%x (%s, %s)",
        (int)video_resolution,
        video_fullscreen ? "fullscreen" : "windowed",
        video_smooth ? "smooth" : "plain"
    );

    /* Create a display */
    if(display == NULL) {
        v2d_t window_size = video_get_window_size();

        /* setup display flags */
        al_set_new_display_flags(ALLEGRO_OPENGL);
        al_set_new_display_flags(ALLEGRO_PROGRAMMABLE_PIPELINE);
        al_set_new_display_flags(video_fullscreen ? ALLEGRO_FULLSCREEN_WINDOW : ALLEGRO_WINDOWED);
        al_set_new_display_option(ALLEGRO_COLOR_SIZE, suggested_bpp, ALLEGRO_SUGGEST);

        /* setup orientation */
        if(window_size.x >= window_size.y)
            al_set_new_display_option(ALLEGRO_SUPPORTED_ORIENTATIONS, ALLEGRO_DISPLAY_ORIENTATION_LANDSCAPE, ALLEGRO_SUGGEST);
        else
            al_set_new_display_option(ALLEGRO_SUPPORTED_ORIENTATIONS, ALLEGRO_DISPLAY_ORIENTATION_PORTRAIT, ALLEGRO_SUGGEST);

        /* create display */
        if(NULL == (display = al_create_display(window_size.x, window_size.y)))
            fatal_error("Failed to create a %dx%d display", (int)window_size.x, (int)window_size.y);
        al_register_event_source(a5_event_queue, al_get_display_event_source(display));
        al_set_window_title(display, WINDOW_TITLE);
        al_hide_mouse_cursor(display);
    }

    /* Compute the dimensions of the screen (gameplay area) */
    screen_size = DEFAULT_SCREEN_SIZE;
    if(resolution == VIDEORESOLUTION_EDT) {
        /* in the level editor, we set screen_size to the dimensions of the display */
        screen_size = v2d_new(al_get_display_width(display), al_get_display_height(display));
    }

    /* toggle fullscreen */
    if(!al_set_display_flag(display, ALLEGRO_FULLSCREEN_WINDOW, video_fullscreen)) {
        logfile_message("Failed to toggle to %s mode", video_fullscreen ? "fullscreen" : "windowed");
        video_fullscreen = prev_fullscreen;
    }

    /* resize the display */
    if(!video_fullscreen) {
        v2d_t window_size = video_get_window_size();
        if(!al_resize_display(display, window_size.x, window_size.y))
            logfile_message("Failed to resize the display to %dx%d", (int)window_size.x, (int)window_size.y);
    }

    /* Create the backbuffer */
    if(backbuffer != NULL)
        image_destroy(backbuffer);
    al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
    if(NULL == (backbuffer = image_create(screen_size.x, screen_size.y)))
        fatal_error("Failed to create a %dx%d backbuffer", (int)screen_size.x, (int)screen_size.y);
    al_set_target_bitmap(IMAGE2BITMAP(backbuffer));
    al_clear_to_color(al_map_rgb(0, 0, 0));
}


/*
 * video_get_resolution()
 * Returns the current resolution
 */
videoresolution_t video_get_resolution()
{
    return video_resolution;
}

/*
 * video_initial_resolution()
 * The initial video resolution (adopted when
 * no resolution has been selected yet)
 */
videoresolution_t video_initial_resolution()
{
    ALLEGRO_MONITOR_INFO info;

    if(al_get_monitor_info(0, &info)) {
        v2d_t desktop = v2d_new(info.x2 - info.x1, info.y2 - info.y1);
        v2d_t screen3 = v2d_multiply(DEFAULT_SCREEN_SIZE, 3);

        if(desktop.x > screen3.x && desktop.y > screen3.y)
            return VIDEORESOLUTION_3X;
        else
            return VIDEORESOLUTION_2X;
    }
    else
        return VIDEORESOLUTION_2X;
}


/*
 * video_is_smooth()
 * Smooth graphics?
 */
bool video_is_smooth()
{
    return video_smooth;
}


/*
 * video_is_fullscreen()
 * Fullscreen mode?
 */
bool video_is_fullscreen()
{
    return video_fullscreen;
}


/*
 * video_get_screen_size()
 * Returns the size of the screen.
 */
v2d_t video_get_screen_size()
{
    return screen_size;
}


/*
 * video_get_window_size()
 * Returns the window size, based on
 * the current resolution
 */
v2d_t video_get_window_size()
{
    int width = DEFAULT_SCREEN_SIZE.x;
    int height = DEFAULT_SCREEN_SIZE.y;

    switch(video_resolution) {
        case VIDEORESOLUTION_1X:
            break;

        case VIDEORESOLUTION_2X:
            width *= 2;
            height *= 2;
            break;

        case VIDEORESOLUTION_3X:
            width *= 3;
            height *= 3;
            break;

        case VIDEORESOLUTION_4X:
            width *= 4;
            height *= 4;
            break;

        case VIDEORESOLUTION_EDT:
            width = VIDEO_SCREEN_W;
            height = VIDEO_SCREEN_H;
            break;
    }

    return v2d_new(width, height);
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
 * video_render()
 * Updates the video manager and the screen
 */
void video_render()
{
    ALLEGRO_STATE state;
    uint32_t current_time;
    static uint32_t fps_timer = 0, frame_count = 0;

    /* video message */
    videomsg = videomsg_render(videomsg, 0);

    /* compute fps rate */
    ++frame_count;
    if((current_time = timer_get_ticks()) >= fps_timer + 1000) {
        fps_timer = current_time;
        fps_rate = frame_count;
        frame_count = 0;
    }

    /* show fps */
    if(video_is_fps_visible())
        PRINT(image_width(backbuffer), 0.0f, ALLEGRO_ALIGN_RIGHT, "%d", fps_rate);

    /* render */
    al_store_state(&state, ALLEGRO_STATE_TRANSFORM | ALLEGRO_STATE_TARGET_BITMAP);
    al_set_target_bitmap(al_get_backbuffer(display));
    apply_display_transform(display, backbuffer);
    al_clear_to_color(al_map_rgb(0, 0, 0));
    al_draw_bitmap(IMAGE2BITMAP(backbuffer), 0.0f, 0.0f, 0);
    al_flip_display();
    al_restore_state(&state);
    /*al_set_target_bitmap(IMAGE2BITMAP(backbuffer));*/
}


/*
 * video_release()
 * Releases the video manager
 */
void video_release()
{
    logfile_message("Releasing the video...");

    if(videomsg != NULL)
        videomsg = videomsg_delete(videomsg);

    if(font != NULL) {
        al_destroy_font(font);
        font = NULL;
    }

    if(backbuffer != NULL) {
        image_destroy(backbuffer);
        backbuffer = NULL;
    }

    if(display != NULL) {
        al_destroy_display(display);
        display = NULL;
    }
}


/*
 * video_showmessage()
 * Shows a text message to the user
 */
void video_showmessage(const char *fmt, ...)
{
    char message[512] = "";
    va_list args;

    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    videomsg = videomsg_new(message, videomsg);
}


/*
 * video_get_color_depth()
 * Returns the current color depth
 */
int video_get_color_depth()
{
    return display ? al_get_display_option(display, ALLEGRO_COLOR_SIZE) : 0;
}


/*
 * video_get_preferred_color_depth()
 * Returns the preferred color depth for the game
 */
int video_get_preferred_color_depth()
{
    return 32;
}


/*
 * video_show_fps()
 * Shows/hides the FPS counter
 */
void video_show_fps(bool show)
{
    video_showfps = show;
}


/*
 * video_is_fps_visible()
 * Is the FPS counter visible?
 */
bool video_is_fps_visible()
{
    return video_showfps;
}

/*
 * video_fps()
 * Get FPS rate
 */
int video_fps()
{
    return fps_rate;
}


/*
 * video_display_loading_screen()
 * Displays a loading screen
 */
void video_display_loading_screen()
{
    static const char* LOADING_IMAGE = "images/loading.png";
    static const char* LOADING_FONT = "Loading";
    image_t *img = image_load(LOADING_IMAGE);
    font_t* fnt = font_create(LOADING_FONT);
    v2d_t cam = v2d_multiply(video_get_screen_size(), 0.5f);

    image_clear(color_rgb(0, 0, 0));
    image_blit(img, 0, 0, (VIDEO_SCREEN_W - image_width(img)) / 2, (VIDEO_SCREEN_H - image_height(img)) / 2, image_width(img), image_height(img));
    font_set_align(fnt, FONTALIGN_CENTER);
    font_set_text(fnt, "%s", lang_get("LOADING_TEXT"));
    font_set_position(fnt, v2d_subtract(cam, v2d_new(0, font_get_textsize(fnt).y / 2)));
    font_render(fnt, cam);
    video_render();
    
    font_destroy(fnt);
}



/* private stuff */

/* apply a transform when rendering, so that we have a resized
   screen that maintains the original aspect ratio of the game */
void apply_display_transform(ALLEGRO_DISPLAY* display, image_t* backbuffer)
{
    ALLEGRO_TRANSFORM t;
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
    al_build_transform(&t, offset.x, offset.y, min(scale.x, scale.y), min(scale.x, scale.y), 0.0f);
    al_use_transform(&t);
}

/* sets the icon of the display to a built-in icon */
void set_display_icon(ALLEGRO_DISPLAY* display)
{
    extern const unsigned char ICON_PNG[];
    extern const size_t ICON_SIZE;
    ALLEGRO_FILE* f = al_open_memfile((void*)ICON_PNG, ICON_SIZE, "r");
    ALLEGRO_BITMAP* icon = al_load_bitmap_f(f, ".png");
    al_set_display_icon(display, icon);
    al_destroy_bitmap(icon);
    al_fclose(f);
}

/* creates a new videomsg_t node */
videomsg_t* videomsg_new(const char* message, videomsg_t* next)
{
    videomsg_t* node = mallocx(sizeof *node);
    node->message = str_dup(message);
    node->endtime = timer_get_ticks() + VIDEOMSG_TIMEOUT;
    node->next = next;
    return node;
}

/* deletes an existing videomsg_t node */
videomsg_t* videomsg_delete(videomsg_t* videomsg)
{
    if(videomsg->next)
        videomsg->next = videomsg_delete(videomsg->next);
    free(videomsg->message);
    free(videomsg);
    return NULL;
}

/* updates and renders a videomsg_t linked list; returns the updated list */
videomsg_t* videomsg_render(videomsg_t* videomsg, int line)
{
    if(videomsg != NULL) {
        /* got timeout? */
        if(timer_get_ticks() >= videomsg->endtime || line + 1 > VIDEOMSG_MAXLINES)
            return videomsg_delete(videomsg);

        /* render current message */
        PRINT(0.0f, image_height(backbuffer) - al_get_font_line_height(font) * (line + 1), ALLEGRO_ALIGN_LEFT, "%s", videomsg->message);

        /* render next message */
        videomsg->next = videomsg_render(videomsg->next, line + 1);
    }

    /* done! */
    return videomsg;
}
