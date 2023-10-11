/*
 * Open Surge Engine
 * waterfx.c - water special effect
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

#include <math.h>
#include "waterfx.h"
#include "../core/image.h"
#include "../core/video.h"
#include "../core/shader.h"
#include "../entities/player.h"
#include "../scenes/level.h"
#include "../util/util.h"

/* shader */
static const char watershader_glsl[] = ""
    FRAGMENT_SHADER_GLSL_PREFIX

    "precision lowp float;\n"

    "uniform sampler2D tex;\n"
    "uniform highp float scroll_y;\n"
    "uniform vec4 watercolor;\n"

#if 0
    "const int wave[48] = int[48](\n"
    "   0,0,0,0,0,0,0,0,\n"
    "   0,0,0,0,0,0,0,0,\n"
    "   1,1,1,1,1,1,1,1,\n"
    "   2,2,2,2,2,2,2,2,\n"
    "   2,2,2,2,2,2,2,2,\n"
    "   1,1,1,1,1,1,1,1 \n"
    ");\n"
#elif 0
    "const int wave[60] = int[60](\n"
    "   0,0,0,0,0,0,0,0,0,0,\n"
    "   0,0,0,0,0,0,0,0,0,0,\n"
    "   1,1,1,1,1,1,1,1,1,1,\n"
    "   2,2,2,2,2,2,2,2,2,2,\n"
    "   2,2,2,2,2,2,2,2,2,2,\n"
    "   1,1,1,1,1,1,1,1,1,1 \n"
    ");\n"
#else
    "const int wave[72] = int[72](\n"
    "   0,0,0,0,0,0,0,0,0,0,0,0,\n"
    "   0,0,0,0,0,0,0,0,0,0,0,0,\n"
    "   1,1,1,1,1,1,1,1,1,1,1,1,\n"
    "   2,2,2,2,2,2,2,2,2,2,2,2,\n"
    "   2,2,2,2,2,2,2,2,2,2,2,2,\n"
    "   1,1,1,1,1,1,1,1,1,1,1,1 \n"
    ");\n"
#endif

    "void main()\n"
    "{\n"
    "   vec4 pixel[3];\n"

    "   pixel[0] = textureOffset(tex, texcoord, ivec2(-1,0));\n"
    "   pixel[1] = textureOffset(tex, texcoord, ivec2(0,0));\n"
    "   pixel[2] = textureOffset(tex, texcoord, ivec2(1,0));\n"

    "   mediump float screen_height = float(textureSize(tex, 0).y);\n"
    "   mediump float screen_y = (1.0 - texcoord.y) * screen_height;\n"
    "   highp int wanted_y = int(screen_y + scroll_y);\n" /* from screen space to world space */
    "   int w = abs(wanted_y) % wave.length();\n"
    "   int k = wave[w];\n"
    "   vec4 wanted_pixel = pixel[k];\n"

    "   vec3 blended_pixel = mix(wanted_pixel.rgb, watercolor.rgb, watercolor.a);\n"
    "   color = vec4(blended_pixel, 1.0);\n"
    "}\n"
;

/* utils */
#define DEFAULT_WATERLEVEL      LARGE_INT
#define DEFAULT_WATERCOLOR()    color_rgba(0, 64, 255, 128)
static int waterlevel = DEFAULT_WATERLEVEL;
static color_t watercolor;
static shader_t* watershader;
static image_t* backbuffer;
static void render_simple_effect(int y, color_t color);
static void render_default_effect(int y, float camera_y, float offset, float timer, color_t color);
static float* color_to_vec4(color_t color, float* vec4);



/*
 * waterfx_init()
 * Initialize the water effect
 */
void waterfx_init()
{
    waterlevel = DEFAULT_WATERLEVEL;
    watercolor = DEFAULT_WATERCOLOR();
    watershader = shader_create("waterfx", watershader_glsl);
    backbuffer = image_create_backbuffer(VIDEO_SCREEN_W, VIDEO_SCREEN_H, false);
}

/*
 * waterfx_release()
 * Release the water effect
 */
void waterfx_release()
{
    image_destroy(backbuffer);
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
    if(player != NULL && !player_is_frozen(player)) {
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
        render_default_effect(y, topleft.y, 0.0f, level_time(), watercolor);
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
        color_t transparent = color_rgba(0, 0, 0, 0);
        render_default_effect(y, 0.0f, 18.0f, 2.0f * level_time(), transparent);
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

/* render a simple water effect
   y >= 0 is given in screen space */
void render_simple_effect(int y, color_t color)
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
    color_t premul_color = color_premul_rgba(red, green, blue, alpha);

    /* render the water */
    v2d_t screen_size = video_get_screen_size();
    image_rectfill(0, y, screen_size.x, screen_size.y, premul_color);
}

/* render the default water effect */
void render_default_effect(int y, float camera_y, float offset, float timer, color_t color)
{
    /* copy the backbuffer */
    image_t* target = image_drawing_target();
    image_set_drawing_target(backbuffer);
    {
        image_clear(color_rgba(0, 0, 0, 0));
        image_draw(video_get_backbuffer(), 0, 0, IF_NONE);
    }
    image_set_drawing_target(target);

    /* scrolling */
    const float speed = 32.0f; /* px/s */
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
        image_blit(backbuffer, 0, y, 0, y, screen_size.x, screen_size.y);
    }
    shader_set_active(prev);
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