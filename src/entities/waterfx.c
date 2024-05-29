/*
 * Open Surge Engine
 * waterfx.c - water special effect
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

#include <math.h>
#include "waterfx.h"
#include "../core/image.h"
#include "../core/video.h"
#include "../core/shader.h"
#include "../core/timer.h"
#include "../core/logfile.h"
#include "../core/engine.h"
#include "../entities/player.h"
#include "../scenes/level.h"
#include "../util/util.h"

/* shader */
static const char watershader_glsl[] = ""
    FRAGMENT_SHADER_GLSL_PREFIX

    /* some drivers crash with a message:
       "precision statement not allowed for type" */
    /*"uniform lowp sampler2D tex;\n"*/

    "uniform sampler2D tex;\n"
    "uniform vec4 watercolor;\n"
    "uniform highp float scroll_y;\n"

    "void main()\n"
    "{\n"
    "   mat4 pixel;\n"

    "   pixel[0] = textureOffset(tex, texcoord, ivec2(-1,0));\n"
    "   pixel[1] = textureOffset(tex, texcoord, ivec2(0,0));\n"
    "   pixel[2] = textureOffset(tex, texcoord, ivec2(1,0));\n"

#if defined(__ANDROID__)
    /*
     * Due to a rule set in the Android version of Allegro 5.2.8, more precisely
     * at src/opengl/ogl_bitmap.c (_al_ogl_create_bitmap), the actual size of
     * the input OpenGL texture may be larger than the size of its corresponding
     * ALLEGRO_BITMAP, with no way to correct it in the API. The result is that
     * the u,v wrapping behavior doesn't work as expected. This creates artifacts
     * at the right edge of the screen, when we read pixels that are out of bounds.
     *
     * This little hack fixes the undesirable behavior by assuming that the input
     * texture has been previously cleared to RGBA (0,0,0,0) and that we have set
     * its GL_TEXTURE_WRAP_[UV] parameter to GL_MIRRORED_REPEAT.
     */
    "   pixel[2] += float(all(equal(pixel[2], vec4(0.0)))) * pixel[0];\n" /* pixel[2] becomes pixel[0] if it's zero */
#endif

    "   highp float screen_height = float(textureSize(tex, 0).y);\n"
    "   highp float screen_y = screen_height - texcoord.y * screen_height;\n"
    "   highp float world_y = screen_y + scroll_y;\n" /* from screen space to world space */
    "   highp int wanted_y = int(abs(world_y));\n"

#if 0
        /* golden ratio */
    "   const int wave[64] = int[64](\n"
    "      0,0,0,0,0,0,0,0,0,0,\n"
    "      0,0,0,0,0,0,0,0,0,0,\n"
    "      1,1,1,1,1,1,1,1,1,1,1,1,\n"
    "      2,2,2,2,2,2,2,2,2,2,\n"
    "      2,2,2,2,2,2,2,2,2,2,\n"
    "      1,1,1,1,1,1,1,1,1,1,1,1\n"
    "   );\n"

        /* slower version; I left it here for clarity */
    "   int w = wave[wanted_y & 63];\n"
    "   vec3 wanted_pixel = pixel[w].rgb;\n"
#else
        /* faster version */
    "   int k = wanted_y & 63;\n"
    "   int w = int(k >= 20) + int(k >= 32) * int(k <= 51);\n" /* waveform (w = 0, 1, 2) */
    "   bvec3 selector = bvec3((w == 0), (w == 1), (w == 2));\n"
    "   vec3 wanted_pixel = mat3(pixel) * vec3(selector);\n"
#endif

    "   vec3 blended_pixel = mix(wanted_pixel, watercolor.rgb, watercolor.a);\n"
    "   color = vec4(blended_pixel, 1.0);\n"
    "}\n"
;

/* backbuffers for post-processing */
#define NUMBER_OF_BACKBUFFERS   2 /* bg, fg */
static image_t* backbuffer[NUMBER_OF_BACKBUFFERS] = { NULL, NULL };
static int backbuffer_index = 0;
static bool create_backbuffers();
static void destroy_backbuffers();
static void handle_video_event(const ALLEGRO_EVENT* event, void* context);

/* internals */
#define DEFAULT_WATERLEVEL      LARGE_INT
#define DEFAULT_WATERCOLOR()    color_rgba(0, 64, 255, 128)
static int waterlevel = DEFAULT_WATERLEVEL;
static color_t watercolor;
static float internal_timer;
static shader_t* watershader;
static void render_simple_effect(int y, color_t color);
static void render_default_effect(int y, float camera_y, float offset, float timer, float speed, color_t color);
static float* color_to_vec4(color_t color, float* vec4);
static color_t premultiply_alpha(color_t color);

/* log */
#define LOG(...)                logfile_message("Waterfx: " __VA_ARGS__)



/*
 * waterfx_init()
 * Initialize the water effect
 */
void waterfx_init()
{
    /* set default values */
    internal_timer = 0.0f;
    waterlevel = DEFAULT_WATERLEVEL;
    watercolor = DEFAULT_WATERCOLOR();

    /* create the shader */
    watershader = shader_create("waterfx", watershader_glsl);

    /* create the backbuffers used for post-processing */
    backbuffer_index = 0;
    if(!create_backbuffers())
        LOG("Can't create the backbuffers!");

    /* add event listeners */
    engine_add_event_listener(ALLEGRO_EVENT_DISPLAY_HALT_DRAWING, NULL, handle_video_event);
    engine_add_event_listener(ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING, NULL, handle_video_event);
}

/*
 * waterfx_release()
 * Release the water effect
 */
void waterfx_release()
{
    /* remove event listeners */
    engine_remove_event_listener(ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING, NULL, handle_video_event);
    engine_remove_event_listener(ALLEGRO_EVENT_DISPLAY_HALT_DRAWING, NULL, handle_video_event);

    /* destroy backbuffers */
    destroy_backbuffers();
}

/*
 * waterfx_update()
 * Update the water effect
 */
void waterfx_update()
{
    float dt = timer_get_delta();
    internal_timer += dt;
}

/*
 * waterfx_render_fg()
 * Render the water effect (foreground / main)
 */
void waterfx_render_fg(v2d_t camera_position)
{
    /* convert the waterlevel from world space to screen space */
    v2d_t half_screen = v2d_multiply(video_get_screen_size(), 0.5f);
    v2d_t topleft = v2d_subtract(camera_position, half_screen);
    int y = waterlevel - topleft.y;

    /* clip out */
    if(y >= VIDEO_SCREEN_H)
        return;

    /* adjust y */
    y = max(0, y);

    /* if the active player is too fast,
       maybe a simple effect will look better? */
    const player_t* player = level_player();
    if(player != NULL && !player_is_dying(player) && !player_is_frozen(player)) {
        static bool disabled_effect = false;
        float abs_ysp = fabsf(player_ysp(player));

        if(disabled_effect || abs_ysp >= 270.0f) {
            disabled_effect = (abs_ysp > 180.0f);
            render_simple_effect(y, watercolor);
            return;
        }
    }

    /* render */
    if(video_get_quality() > VIDEOQUALITY_LOW)
        render_default_effect(y, topleft.y, 0.0f, internal_timer, 32.0f, watercolor);
    else
        render_simple_effect(y, watercolor);
}

/*
 * waterfx_render_bg()
 * Render the water effect (background)
 */
void waterfx_render_bg(v2d_t camera_position)
{
    /* convert the waterlevel from world space to screen space */
    v2d_t half_screen = v2d_multiply(video_get_screen_size(), 0.5f);
    v2d_t topleft = v2d_subtract(camera_position, half_screen);
    int y = waterlevel - topleft.y;

    /* clip out */
    if(y >= VIDEO_SCREEN_H)
        return;

    /* adjust y */
    y = max(0, y);

    /* render */
    if(video_get_quality() > VIDEOQUALITY_LOW) {
        float camera_y = 0.0f; /* no camera */
        color_t transparent = color_rgba(0, 0, 0, 0);
        render_default_effect(y, camera_y, 16.0f, internal_timer, 64.0f, transparent);
    }
}

/*
 * waterfx_set_ypos()
 * Set the y-position of the water in world space
 */
void waterfx_set_ypos(int ypos)
{
    waterlevel = ypos;
}

/*
 * waterfx_ypos()
 * Get the y-position of the water in world space
 */
int waterfx_ypos()
{
    return waterlevel;
}

/*
 * waterfx_default_ypos()
 * Get the default y-position of the water in world space
 */
int waterfx_default_ypos()
{
    return DEFAULT_WATERLEVEL;
}

/*
 * waterfx_set_color()
 * Set the color of the water
 */
void waterfx_set_color(color_t color)
{
    watercolor = color;
}

/*
 * waterfx_color()
 * Get the color of the water
 */
color_t waterfx_color()
{
    return watercolor;
}

/*
 * waterfx_default_color()
 * Get the default color of the water
 */
color_t waterfx_default_color()
{
    return DEFAULT_WATERCOLOR();
}



/*
 *
 * private
 *
 */

/* create the backbuffers used for post-processing */
bool create_backbuffers()
{
    for(int i = 0; i < NUMBER_OF_BACKBUFFERS; i++) {
        backbuffer[i] = image_create_ex(VIDEO_SCREEN_W, VIDEO_SCREEN_H, IC_BACKBUFFER | IC_WRAP_MIRROR);
        if(backbuffer[i] == NULL)
            return false;
    }

    return true;
}

/* destroy the backbuffers used for post-processing */
void destroy_backbuffers()
{
    for(int i = NUMBER_OF_BACKBUFFERS - 1; i >= 0; i--) {
        if(backbuffer[i] != NULL) {
            image_destroy(backbuffer[i]);
            backbuffer[i] = NULL;
        }
    }
}

/* handle a video event */
void handle_video_event(const ALLEGRO_EVENT* event, void* context)
{
    switch(event->type) {
        case ALLEGRO_EVENT_DISPLAY_HALT_DRAWING:
            destroy_backbuffers();
            break;

        case ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING:
            if(!create_backbuffers())
                LOG("Can't recreate the backbuffers!");
            break;
    }
}

/* render a simple water effect
   y >= 0 is given in screen space */
void render_simple_effect(int y, color_t color)
{
    v2d_t screen_size = video_get_screen_size();
    image_rectfill(0, y, screen_size.x, screen_size.y, premultiply_alpha(watercolor));
}

/* render the default water effect */
void render_default_effect(int y, float camera_y, float offset, float timer, float speed, color_t color)
{
    /* this should never happen; backbuffers may be recreated */
    if(backbuffer[backbuffer_index] == NULL) {
        /*render_simple_effect(y, color);*/
        return;
    }

    /* empty the command buffer */
    video_flush();

    /* copy the backbuffer */
    /* possibly expensive on mobile platforms because we trigger a pipeline
       flush when unbinding a partially rendered FBO, but we mitigate the cost
       with low-res graphics. */
    image_t* target = image_drawing_target();
    image_set_drawing_target(backbuffer[backbuffer_index]);
    {
        image_clear(color_rgba(0, 0, 0, 0));
        image_draw(video_get_backbuffer(), 0, 0, IF_NONE);
    }
    image_set_drawing_target(target);

    /* scrolling */
    float world_scroll_y = speed * timer + offset;
    float scroll_y = world_scroll_y + camera_y;
    shader_set_float(watershader, "scroll_y", scroll_y);

    /* watercolor */
    float vec4[4];
    shader_set_float_vector(watershader, "watercolor", 4, color_to_vec4(color, vec4));

    /* render */
    const shader_t* prev = shader_get_active();
    shader_set_active(watershader);
    {
        v2d_t screen_size = video_get_screen_size();
        image_blit(backbuffer[backbuffer_index], 0, y, 0, y, screen_size.x, screen_size.y);
    }
    shader_set_active(prev);

    /* round-robin */
    backbuffer_index = (1 + backbuffer_index) % NUMBER_OF_BACKBUFFERS;
}

/* convert a RGBA color to a vec4 in [0,1]^4 */
float* color_to_vec4(color_t color, float* vec4)
{
    uint8_t r, g, b, a;
    color_unmap(color, &r, &g, &b, &a);

    vec4[0] = (float)r / 255.0f;
    vec4[1] = (float)g / 255.0f;
    vec4[2] = (float)b / 255.0f;
    vec4[3] = (float)a / 255.0f;

    return vec4;
}

/* pre-multiply alpha */
color_t premultiply_alpha(color_t color)
{
    /*

    Let's adjust the color of the water by pre-multiplying the alpha value

    "By default Allegro uses pre-multiplied alpha for transparent blending of
    bitmaps and primitives (see al_load_bitmap_flags for a discussion of that
    feature). This means that if you want to tint a bitmap or primitive to be
    transparent you need to multiply the color components by the alpha
    components when you pass them to this function."

    Source: Allegro manual at
    https://liballeg.org/a5docs/trunk/graphics.html#al_premul_rgba

    */

    uint8_t red, green, blue, alpha;
    color_unmap(color, &red, &green, &blue, &alpha);
    return color_premul_rgba(red, green, blue, alpha);
}
