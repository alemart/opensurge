/*
 * Open Surge Engine
 * video.c - video manager
 * Copyright 2008-2024 Alexandre Martins <alemartf(at)gmail.com>
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
#include "config.h"
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
ALLEGRO_DEFINE_PROC_TYPE(const GLubyte*, fun_glgetstring_t, (GLenum));
static fun_glinvalidateframebuffer_t _glInvalidateFramebuffer = NULL;
static fun_glclear_t _glClear = NULL;
static fun_glclearcolor_t _glClearColor = NULL;
static fun_glcleardepth_t _glClearDepth = NULL;
static fun_glgetstring_t _glGetString = NULL;
static void import_opengl_symbols();


/* FPS counter */
#define TARGET_FPS 60
#define FPS_UPDATE_FREQUENCY 1 /* updates per second (ideally) */
#define NUMBER_OF_FPS_SAMPLES (TARGET_FPS / FPS_UPDATE_FREQUENCY)
static double fps = 0.0;
static double fps_median = 0.0;
static double fps_sample[NUMBER_OF_FPS_SAMPLES];
static int index_of_next_fps_sample = 0;
static int fps_frames = 0;
static double fps_counted = 0.0;
static double fps_last_update = 0.0;
static void init_fps();
static void update_fps();
static void render_fps();
static int sort_fps_samples(const void* a, const void* b);


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
    game_screen_width = config_video_screen_width(DEFAULT_SCREEN_WIDTH);
    game_screen_height = config_video_screen_height(DEFAULT_SCREEN_HEIGHT);
    str_cpy(window_title, config_game_title(DEFAULT_WINDOW_TITLE), sizeof(window_title));

    /* initialize the FPS counter */
    init_fps();

    /* create the display */
    if(!create_display(game_screen_width, game_screen_height))
        FATAL("Failed to create a %dx%d display", game_screen_width, game_screen_height);

    /* create the backbuffer */
    if(!create_backbuffer())
        FATAL("Failed to create the backbuffer");

    /* import OpenGL symbols */
    import_opengl_symbols();

    /* now that we have a display (i.e., a valid OpenGL context) and the
       OpenGL symbols, let's log a few more things */
    if(_glGetString != NULL) {
        LOG("GL version: %s", (const char*)_glGetString(GL_VERSION));
        LOG("GL vendor: %s", (const char*)_glGetString(GL_VENDOR));
        LOG("GL renderer: %s", (const char*)_glGetString(GL_RENDERER));
    }

    if(al_get_opengl_variant() == ALLEGRO_OPENGL_ES) /* see remark at video_is_using_gles() */
        LOG("OpenGL ES version 0x%08x", al_get_opengl_version());
    else
        LOG("OpenGL version 0x%08x", al_get_opengl_version());

    /* initialize the shader system */
    shader_init();
    if(!use_default_shader())
        FATAL("Failed to use the default shader");

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

    /* render stuff in window space */
    if(render_overlay != NULL)
        render_overlay();
    render_texts();

    /* flip display */
    al_flip_display();

    /* compute the framerate */
    update_fps();

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

    /* If the engine is being restarted, changing the display flag below causes
       an AllegroActivity.nativeOnChange() call, which in turn triggers an
       ALLEGRO_EVENT_DISPLAY_RESIZE event. Allegro will wait on the main thread
       until the game calls al_acknowledge_resize(), but by that time we'll be
       trying to destroy the display. Allegro will call postDestroySurface(),
       which intends to make the main thread call destroySurface(). The latter
       will never be called, because Allegro is waiting on the main thread.
       Since destroySurface() is not called, AllegroSurface.surfaceDestroyed()
       isn't called either. Therefore, AllegroSurface.nativeOnDestroy() won't
       ever be called and Allegro will freeze on android_destroy_display().
       It will be waiting both on the main thread and in the game thread.
       This causes the game to freeze. Tested with Allegro 5.2.9. */
    if(engine_is_init() && !engine_must_quit() && !engine_must_restart(NULL)) {

        /* change the display flag */
        al_set_display_flag(display, ALLEGRO_FRAMELESS, immersive);

        /* restore the default shader */
        if(changed_mode) {
            if(!use_default_shader())
                LOG("Can't set the default shader");
        }

    }
    else {
        LOG("ERROR: can't change the immersive mode!");
        return;
    }

#else

    /* the immersive mode is unavailable in this platform */

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
    video_display_loading_screen_ex(NAN);
}

/*
 * video_display_loading_screen_ex()
 * Displays a loading screen with a progress bar
 * 0 <= progress <= 1
 */
void video_display_loading_screen_ex(double progress)
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

    /* render the loading screen */
    image_clear(color_rgb(0, 0, 0));
    image_draw(img, (VIDEO_SCREEN_W - image_width(img)) / 2, (VIDEO_SCREEN_H - image_height(img)) / 2, IF_NONE);
    font_render(fnt, camera);

    /* render the progress bar */
    if(!isnan(progress)) {
        double p = clip(progress, 0.0, 1.0);
        int progress_height = VIDEO_SCREEN_H / 60;
        color_t progress_fgcolor = color_rgb(255, 0, 0);
        color_t progress_bgcolor = color_rgba(0, 0, 0, 128);

        image_rectfill(0, 0, VIDEO_SCREEN_W, progress_height, progress_bgcolor);
        image_rectfill(0, 0, VIDEO_SCREEN_W * p, progress_height, progress_fgcolor);
    }

    /* render the backbuffer to the screen */
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

/*
 * video_is_using_gles()
 * Are we using OpenGL ES?
 */
bool video_is_using_gles()
{
#if !defined(__ANDROID__)

    const char* gl_version;

   /* According to the OpenGL ES 3.0.6 spec, section 2.1 page 5,
      "Issuing GL commands when the program is not connected to a context results in undefined behavior." */
    assertx(display != NULL, "need a valid OpenGL context");

    if(NULL == _glGetString || NULL == (gl_version = (const char*)_glGetString(GL_VERSION))) {
        /* fallback. As of Allegro 5.2.9, the return value of
           al_get_opengl_variant() is determined by a preprocessor constant */
        return al_get_opengl_variant() == ALLEGRO_OPENGL_ES;
    }

    /*

    OpenGL 2.0 (sec 6.1.11), 3.0 (sec 6.1.11), 4.0 (sec 6.1.6) specifications:

        The VERSION string is laid out as follows:
        <version number> <vendor-specific information>

    OpenGL ES 2.0 (sec 6.1), 3.0 (sec 6.1.6) specifications:

        The VERSION string is laid out as follows:
        OpenGL ES <version number> <vendor-specific information>

    */

    return strncmp(gl_version, "OpenGL ES", 9) == 0;

#else

    return true;

#endif
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

    al_set_new_window_title(window_title);

    display = al_create_display(width, height);
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
#else
    /* On Android, we can't change any display flag while the engine is restarting.
       The game will freeze if we try. (see an explanation of this at video_set_immersive()) */
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

/* initialize the FPS counter */
void init_fps()
{
    fps = 0.0;

    fps_frames = 0;
    fps_counted = 0.0;
    fps_last_update = 0.0;

    fps_median = 0.0;
    index_of_next_fps_sample = 0;
    for(int i = 0; i < NUMBER_OF_FPS_SAMPLES; i++)
        fps_sample[i] = 0.0;
}

/* sorting function for the framerate samples */
int sort_fps_samples(const void* a, const void* b)
{
    double x = *((double*)a);
    double y = *((double*)b);

    return (x > y) - (x < y);
}

/* compute the framerate */
void update_fps()
{
    double delta_time = timer_get_delta();
    double elapsed_time = timer_get_elapsed();

    /* method 1: count the number of frames */
    ++fps_frames;
    if(elapsed_time >= fps_last_update + 1.0 / FPS_UPDATE_FREQUENCY) {
        fps_last_update = elapsed_time;
        fps_counted = fps_frames * FPS_UPDATE_FREQUENCY;
        fps_frames = 0;
    }

    /* method 2: take the median of the samples of the framerate */
    if(index_of_next_fps_sample >= NUMBER_OF_FPS_SAMPLES) {
        qsort(fps_sample, NUMBER_OF_FPS_SAMPLES, sizeof(*fps_sample), sort_fps_samples);
        double mid_a = fps_sample[NUMBER_OF_FPS_SAMPLES / 2];
        double mid_b = fps_sample[(NUMBER_OF_FPS_SAMPLES - 1 + NUMBER_OF_FPS_SAMPLES % 2) / 2];
        fps_median = (mid_a + mid_b) / 2.0;
        index_of_next_fps_sample = 0;
    }

    /* collect a sample of the framerate */
    fps_sample[index_of_next_fps_sample++] = 1.0 / delta_time;

    /* Compare the two methods of determining the framerate. If their results
       are very similar, take the median of the samples. If they are not, the
       dataset has outliers. Let's take the counted method in this case. */
    if(fabs(fps_median - fps_counted) < 2)
        fps = fps_median; /* usually ~60 */
    else
        fps = fps_counted; /* usually 59, 60, 61 */
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
        DRAW_TEXT(0.0f, 0.0f, ALLEGRO_ALIGN_RIGHT, "%.1lf", fps);
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

    /* glGetString */
    _glGetString = (fun_glgetstring_t)al_get_opengl_proc_address("glGetString");
#else
    _glClear = NULL;
    _glClearColor = NULL;
    _glClearDepth = NULL;
    _glInvalidateFramebuffer = NULL;
    _glGetString = NULL;
#endif

    /* log */
    #define LOG_GL(symbol) LOG("Found " #symbol ": %s", (_ ## symbol != NULL) ? "yes" : "no")

    LOG("Importing OpenGL symbols");
    LOG_GL(glClear);
    LOG_GL(glClearColor);
    LOG_GL(glClearDepth);
    LOG_GL(glInvalidateFramebuffer);
    LOG_GL(glGetString);

    #undef LOG_GL
}
