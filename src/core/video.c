/*
 * Open Surge Engine
 * video.c - video manager
 * Copyright (C) 2008-2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#if defined(A5BUILD)

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>

#define IMAGE2BITMAP(img) (*((ALLEGRO_BITMAP**)(img)))
#define PRINT(x, y, flags, fmt, ...) do { \
    al_draw_textf(font, al_map_rgb(0, 0, 0), (x) + 1.0f, (y) + 1.0f, (flags) | ALLEGRO_ALIGN_INTEGER, (fmt), __VA_ARGS__); \
    al_draw_textf(font, al_map_rgb(0, 0, 0), (x) + 0.0f, (y) + 1.0f, (flags) | ALLEGRO_ALIGN_INTEGER, (fmt), __VA_ARGS__); \
    al_draw_textf(font, al_map_rgb(255, 255, 255), (x), (y), (flags) | ALLEGRO_ALIGN_INTEGER, (fmt), __VA_ARGS__); \
} while(0)

static ALLEGRO_DISPLAY* display = NULL;
static image_t* backbuffer = NULL;
static ALLEGRO_FONT* font = NULL;

static void setup_color_depth(int bpp);

#else

#include <png.h>
#include <allegro.h>
#include <loadpng.h>
#include <jpgalleg.h>
#include "hqx/hqx.h"

#define IMAGE2BITMAP(img)       (*((BITMAP**)(img)))   /* whoooa, this is crazy stuff */
static image_t *video_buffer;
static image_t *window_surface;

static void fast2x_blit(image_t *src, image_t *dest);
static void smooth2x_blit(image_t *src, image_t *dest);
static void smooth3x_blit(image_t *src, image_t *dest);
static void smooth4x_blit(image_t *src, image_t *dest);
static void window_switch_in();
static void window_switch_out();
static int window_active = TRUE;
static void draw_to_screen(image_t *img);
static void setup_color_depth(int bpp);

#endif

/* private stuff */
#define DEFAULT_SCREEN_SIZE     (v2d_t){ 426, 240 }    /* this is set on stone! Picked a 16:9 resolution */
#define LOADING_SCREEN_FILE     "images/loading.png"

/* video manager */
static v2d_t screen_size = DEFAULT_SCREEN_SIZE; /* represents the size of the screen. This may change (eg, is the user on the level editor?) */
static videoresolution_t video_resolution = VIDEORESOLUTION_1X;
static int video_fullscreen = FALSE;
static int video_showfps = FALSE;
static int video_smooth = FALSE;
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
void video_init(videoresolution_t resolution, int smooth, int fullscreen, int bpp)
{
#if defined(A5BUILD)
    logfile_message("Initializing the video...");
    setup_color_depth(bpp);

    /* video init */
    display = NULL;
    backbuffer = NULL;
    video_changemode(resolution, smooth, fullscreen);

    /* console font */
    font = al_create_builtin_font();

    /* video message */
    videomsg = NULL;
#else
    logfile_message("video_init()");
    setup_color_depth(bpp);

    /* initializing addons */
    logfile_message("Initializing JPGalleg...");
    jpgalleg_init();
    logfile_message("Initializing loadpng...");
    loadpng_init();

    /* video init */
    video_buffer = NULL;
    window_surface = NULL;
    video_changemode(resolution, smooth, fullscreen);

    /* window properties */
    LOCK_FUNCTION(game_quit);
    set_close_button_callback(game_quit);
    set_window_title(GAME_TITLE " " GAME_VERSION_STRING);

    /* window callbacks */
    window_active = TRUE;
    if(set_display_switch_mode(SWITCH_BACKGROUND) == 0) {
        if(set_display_switch_callback(SWITCH_IN, window_switch_in) != 0)
            logfile_message("can't set_display_switch_callback(SWTICH_IN, window_switch_in)");

        if(set_display_switch_callback(SWITCH_OUT, window_switch_out) != 0)
            logfile_message("can't set_display_switch_callback(SWTICH_OUT, window_switch_out)");
    }
    else
        logfile_message("can't set_display_switch_mode(SWITCH_BACKGROUND)");

    /* video message */
    videomsg = NULL;
#endif
}

/*
 * video_changemode()
 * Sets up the game window
 */
void video_changemode(videoresolution_t resolution, int smooth, int fullscreen)
{
#if defined(A5BUILD)
    extern ALLEGRO_EVENT_QUEUE* a5_event_queue;
    v2d_t window_size = video_get_window_size();

    /* Change the video mode */
    logfile_message("video_changemode(%d,%d,%d)", (int)resolution, smooth, fullscreen);
    video_resolution = resolution;
    video_fullscreen = FALSE; /* TODO */
    video_smooth = FALSE; /* not supported yet */
    if(video_resolution != VIDEORESOLUTION_EDT)
        window_size = video_get_window_size();

    /* Create a display */
    if(display == NULL) {
        al_set_new_display_flags(ALLEGRO_OPENGL);
        al_set_new_display_flags(ALLEGRO_PROGRAMMABLE_PIPELINE);
        al_set_new_display_flags(video_fullscreen ? ALLEGRO_FULLSCREEN_WINDOW : ALLEGRO_WINDOWED);
        if(window_size.x >= window_size.y)
            al_set_new_display_option(ALLEGRO_SUPPORTED_ORIENTATIONS, ALLEGRO_DISPLAY_ORIENTATION_LANDSCAPE, ALLEGRO_SUGGEST);
        else
            al_set_new_display_option(ALLEGRO_SUPPORTED_ORIENTATIONS, ALLEGRO_DISPLAY_ORIENTATION_PORTRAIT, ALLEGRO_SUGGEST);
        if(NULL == (display = al_create_display(window_size.x, window_size.y)))
            fatal_error("Failed to create a %dx%d display", (int)window_size.x, (int)window_size.y);
        al_register_event_source(a5_event_queue, al_get_display_event_source(display));
        al_set_window_title(display, GAME_TITLE " " GAME_VERSION_STRING " [A5]");
    }
    else {
        if(!al_resize_display(display, window_size.x, window_size.y))
            logfile_message("Failed to resize the display to %dx%d", (int)window_size.x, (int)window_size.y);
        if(!al_set_display_flag(display, ALLEGRO_FULLSCREEN_WINDOW, video_fullscreen))
            logfile_message("Failed to toggle to %s mode", video_fullscreen ? "fullscreen" : "windowed");
    }

    /* Create the backbuffer */
    screen_size = (resolution == VIDEORESOLUTION_EDT) ? window_size : DEFAULT_SCREEN_SIZE;
    if(backbuffer != NULL)
        image_destroy(backbuffer);
    al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
    if(NULL == (backbuffer = image_create(screen_size.x, screen_size.y)))
        fatal_error("Failed to create a %dx%d backbuffer", (int)screen_size.x, (int)screen_size.y);
    al_set_target_bitmap(IMAGE2BITMAP(backbuffer));
    al_clear_to_color(al_map_rgb(0, 0, 0));
#else
    int width, height;
    int mode;

    logfile_message("video_changemode(%d,%d,%d)", (int)resolution, smooth, fullscreen);

    /* resolution */
    screen_size = (resolution == VIDEORESOLUTION_EDT) ? video_get_window_size() : DEFAULT_SCREEN_SIZE;
    video_resolution = resolution;

    /* fullscreen */
    video_fullscreen = fullscreen;

    /* smooth graphics? */
    video_smooth = smooth;
    if(video_smooth) {
        if(video_get_color_depth() != 32) {
            logfile_message("smooth graphics can only be enabled when using 32 bits per pixel (currently, we're using %d bpp)", video_get_color_depth());
            video_smooth = FALSE;
        }
        else if(video_resolution == VIDEORESOLUTION_1X || video_resolution == VIDEORESOLUTION_EDT) {
            logfile_message("can't enable smooth graphics using resolution %d", (int)video_resolution);
            video_smooth = FALSE;
        }
        else {
            logfile_message("initializing hqx...");
            hqxInit();
        }
    }

    /* creating the backbuffer... */
    logfile_message("creating the backbuffer...");
    if(video_buffer != NULL)
        image_destroy(video_buffer);
    video_buffer = image_create(VIDEO_SCREEN_W, VIDEO_SCREEN_H);

    /* creating the window surface... */
    logfile_message("creating the window surface...");
    if(window_surface != NULL)
        image_destroy(window_surface);
    window_surface = image_create((int)(video_get_window_size().x), (int)(video_get_window_size().y));

    /* setting up the window... */
    logfile_message("setting up the window...");
    mode = video_fullscreen ? GFX_AUTODETECT : GFX_AUTODETECT_WINDOWED;
    #ifdef _WIN32
    width = (int)(video_get_window_size().x) - (int)(video_get_window_size().x) % 4; /* A4 bug? let width be a multiple of 4 */
    #else
    width = (int)(video_get_window_size().x);
    #endif
    height = (int)(video_get_window_size().y);
    if(set_gfx_mode(mode, width, height, 0, 0) < 0)
        fatal_error("video_changemode(): couldn't set the graphic mode (%dx%d)!\n%s", width, height, allegro_error);

    /* done! */
    logfile_message("video_changemode() ok");
#endif
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
 * video_is_smooth()
 * Smooth graphics?
 */
int video_is_smooth()
{
    return video_smooth;
}


/*
 * video_is_fullscreen()
 * Fullscreen mode?
 */
int video_is_fullscreen()
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
#if defined(A5BUILD)
    return backbuffer;
#else
    if(video_buffer == NULL)
        fatal_error("FATAL ERROR: video_get_backbuffer() returned NULL!");

    return video_buffer;
#endif
}

/*
 * video_render()
 * Updates the video manager and the screen
 */
void video_render()
{
#if defined(A5BUILD)
    static uint32_t fps_timer = 0, frame_count = 0;
    uint32_t current_time;

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
    al_set_target_bitmap(al_get_backbuffer(display));
    switch(video_get_resolution()) {
        case VIDEORESOLUTION_1X:
        case VIDEORESOLUTION_EDT:
            al_draw_bitmap(IMAGE2BITMAP(backbuffer), 0.0f, 0.0f, 0);
            break;

        default:
            /* TODO: smooth gfx */
            al_draw_scaled_bitmap(IMAGE2BITMAP(backbuffer),
                0.0f, 0.0f, image_width(backbuffer), image_height(backbuffer),
                0.0f, 0.0f, al_get_bitmap_width(al_get_target_bitmap()), al_get_bitmap_height(al_get_target_bitmap()),
            0);
            break;
    }
    al_flip_display();
    al_set_target_bitmap(IMAGE2BITMAP(backbuffer));
#else
    static uint32_t fps_timer = 0, frame_count = 0;
    uint32_t current_time;

    /* video message */
    videomsg = videomsg_render(videomsg, 0);

    /* compute fps rate */
    ++frame_count;
    if((current_time = timer_get_ticks()) >= fps_timer + 1000) {
        fps_timer = current_time;
        fps_rate = frame_count;
        frame_count = 0;
    }

    /* fps counter */
    if(video_is_fps_visible())
        textprintf_right_ex(IMAGE2BITMAP(video_get_backbuffer()), font, VIDEO_SCREEN_W, 0, makecol(255,255,255), makecol(0,0,0),"FPS:%3d", fps_rate);

    /* render */
    switch(video_get_resolution()) {
        /* tiny window */
        case VIDEORESOLUTION_1X:
        {
            draw_to_screen(video_get_backbuffer());
            break;
        }

        /* double size */
        case VIDEORESOLUTION_2X:
        {
            image_t *tmp = window_surface;

            if(!video_is_smooth())
                fast2x_blit(video_get_backbuffer(), tmp);
            else
                smooth2x_blit(video_get_backbuffer(), tmp);

            draw_to_screen(tmp);
            break;
        }

        /* triple size */
        case VIDEORESOLUTION_3X:
        {
            image_t *tmp = window_surface;

            if(!video_is_smooth()) {
                image_t *src = video_get_backbuffer();
                stretch_blit(IMAGE2BITMAP(src), IMAGE2BITMAP(tmp), 0, 0, image_width(src), image_height(src), 0, 0, image_width(tmp), image_height(tmp));
            }
            else
                smooth3x_blit(video_get_backbuffer(), tmp);

            draw_to_screen(tmp);
            break;
        }

        /* quadruple size */
        case VIDEORESOLUTION_4X:
        {
            image_t *tmp = window_surface;

            if(!video_is_smooth()) {
                image_t *src = video_get_backbuffer();
                stretch_blit(IMAGE2BITMAP(src), IMAGE2BITMAP(tmp), 0, 0, image_width(src), image_height(src), 0, 0, image_width(tmp), image_height(tmp));
            }
            else
                smooth4x_blit(video_get_backbuffer(), tmp);

            draw_to_screen(tmp);
            break;
        }

        /* level editor */
        case VIDEORESOLUTION_EDT:
        {
            draw_to_screen(video_get_backbuffer());
            break;
        }
    }
#endif
}


/*
 * video_release()
 * Releases the video manager
 */
void video_release()
{
#if defined(A5BUILD)
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
#else
    logfile_message("video_release()");

    if(video_buffer != NULL) {
        image_destroy(video_buffer);
        video_buffer = NULL;
    }

    if(window_surface != NULL) {
        image_destroy(window_surface);
        window_surface = NULL;
    }

    if(videomsg != NULL)
        videomsg = videomsg_delete(videomsg);

    logfile_message("video_release() ok");
#endif
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
#if defined(A5BUILD)
    return display ? al_get_display_option(display, ALLEGRO_COLOR_SIZE) : 0;
#else
    return get_color_depth();
#endif
}


/*
 * video_get_preferred_color_depth()
 * Returns the preferred color depth for the game
 */
int video_get_preferred_color_depth()
{
#if defined(A5BUILD)
    return 32;
#else
    int depth = desktop_color_depth();
    return depth != 0 ? depth : 32;
#endif
}


/*
 * video_is_window_active()
 * Returns TRUE if the game window is active,
 * or FALSE otherwise
 */
int video_is_window_active()
{
#if defined(A5BUILD)
    extern bool a5_display_active;
    return a5_display_active;
#else
    return window_active;
#endif
}



/*
 * video_show_fps()
 * Shows/hides the FPS counter
 */
void video_show_fps(int show)
{
    video_showfps = show;
}


/*
 * video_is_fps_visible()
 * Is the FPS counter visible?
 */
int video_is_fps_visible()
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
    image_t *img = image_load(LOADING_SCREEN_FILE);
    image_blit(img, 0, 0, (VIDEO_SCREEN_W - image_width(img))/2, (VIDEO_SCREEN_H - image_height(img))/2, image_width(img), image_height(img));
    video_render();
}


/*
 * video_get_window_surface()
 * The window surface (read-only)
 */
const image_t* video_get_window_surface()
{
#if defined(A5BUILD)
    return backbuffer; /* FIXME */
#else
    switch(video_get_resolution()) {
        case VIDEORESOLUTION_1X:
        case VIDEORESOLUTION_EDT:
            return video_get_backbuffer(); /* this "gambiarra" saves some processing... */

        default:
            return window_surface;
    }
#endif
}


/* private stuff */
#if defined(A5BUILD)

/* configure the color depth: this must be set before creating the display */
void setup_color_depth(int bpp)
{
    if(!(bpp == 16 || bpp == 24 || bpp == 32))
        fatal_error("Invalid color depth: %d. Valid modes are: 16, 24, 32.", bpp);

    al_set_new_display_option(ALLEGRO_COLOR_SIZE, bpp, ALLEGRO_SUGGEST);
}

#else

/* fast2x_blit resizes the src image by a
 * factor of 2. It assumes that:
 *
 * src is a memory bitmap
 * dest is a previously created memory bitmap
 * ---- width of dest = 2 * width of src
 * ---- height of dest = 2 * height of src */
void fast2x_blit(image_t *src, image_t *dest)
{
    int i, j;

    if(IMAGE2BITMAP(src) == NULL || IMAGE2BITMAP(dest) == NULL)
        return;

    switch(video_get_color_depth())
    {
        case 16: {
            int w = image_width(dest), h = image_height(dest);
            for(j=0; j<h; j++) {
                for(i=0; i<w; i++)
                    ((uint16_t*)IMAGE2BITMAP(dest)->line[j])[i] = ((uint16_t*)IMAGE2BITMAP(src)->line[j/2])[i/2];
            }
            break;
        }

        case 24: {
            stretch_blit(IMAGE2BITMAP(src), IMAGE2BITMAP(dest), 0, 0, image_width(src), image_height(src), 0, 0, image_width(dest), image_height(dest));
            break;
        }

        case 32: {
            int w = image_width(dest), h = image_height(dest);
            for(j=0; j<h; j++) {
                for(i=0; i<w; i++)
                    ((uint32_t*)IMAGE2BITMAP(dest)->line[j])[i] = ((uint32_t*)IMAGE2BITMAP(src)->line[j/2])[i/2];
            }
            break;
        }

        default:
            break;
    }
}

/* applies the hqx algorithm */
void smooth2x_blit(image_t *src, image_t *dest)
{
    if(video_get_color_depth() == 32)
        hq2x_32((uint32_t*)(&(IMAGE2BITMAP(src)->line[0][0])), (uint32_t*)(&(IMAGE2BITMAP(dest)->line[0][0])), image_width(src), image_height(src));
}

void smooth3x_blit(image_t *src, image_t *dest)
{
    if(video_get_color_depth() == 32)
        hq3x_32((uint32_t*)(&(IMAGE2BITMAP(src)->line[0][0])), (uint32_t*)(&(IMAGE2BITMAP(dest)->line[0][0])), image_width(src), image_height(src));
}

void smooth4x_blit(image_t *src, image_t *dest)
{
    if(video_get_color_depth() == 32)
        hq4x_32((uint32_t*)(&(IMAGE2BITMAP(src)->line[0][0])), (uint32_t*)(&(IMAGE2BITMAP(dest)->line[0][0])), image_width(src), image_height(src));
}


/* draws img to the screen */
void draw_to_screen(image_t *img)
{
    if(IMAGE2BITMAP(img) == NULL) {
        logfile_message("Can't use video resolution %d", (int)video_get_resolution());
        video_showmessage("Can't use video resolution %d", (int)video_get_resolution());
        video_changemode(VIDEORESOLUTION_2X, video_is_smooth(), video_is_fullscreen());
    }
    else
        blit(IMAGE2BITMAP(img), screen, 0, 0, 0, 0, image_width(img), image_height(img));
}

/* this window is active */
void window_switch_in()
{
    window_active = TRUE;
}


/* this window is not active */
void window_switch_out()
{
    window_active = FALSE;
}

/* setups the color depth */
void setup_color_depth(int bpp)
{
    if(!(bpp == 16 || bpp == 24 || bpp == 32))
        fatal_error("Invalid color depth: %d. Valid modes are: 16, 24, 32.", bpp);

    set_color_depth(bpp);
    set_color_conversion(COLORCONV_TOTAL);
}
#endif

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
#if defined(A5BUILD)
        PRINT(0.0f, image_height(backbuffer) - al_get_font_line_height(font) * (line + 1), ALLEGRO_ALIGN_LEFT, "%s", videomsg->message);
#else
        textout_ex(IMAGE2BITMAP(video_buffer), font, videomsg->message, 0, image_height(video_buffer) - text_height(font) * (line + 1), makecol(255,255,255), makecol(0,0,0));
#endif

        /* render next message */
        videomsg->next = videomsg_render(videomsg->next, line + 1);
    }

    /* done! */
    return videomsg;
}
