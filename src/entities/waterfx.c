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

#include "waterfx.h"
#include "../core/image.h"
#include "../core/video.h"
#include "../util/util.h"

/* utils */
#define DEFAULT_WATERLEVEL      LARGE_INT
#define DEFAULT_WATERCOLOR()    color_rgba(0, 64, 255, 128)
static int waterlevel = DEFAULT_WATERLEVEL;
static color_t watercolor;
static void render_simple(int y);



/*
 * waterfx_init()
 * Initialize the water effect
 */
void waterfx_init()
{
    waterlevel = DEFAULT_WATERLEVEL;
    watercolor = DEFAULT_WATERCOLOR();
}

/*
 * waterfx_release()
 * Release the water effect
 */
void waterfx_release()
{
    ;
}

/*
 * waterfx_render()
 * Render the water effect
 */
void waterfx_render(v2d_t camera_position)
{
    /* convert the waterlevel from world space to screen space */
    int y = waterlevel - ((int)camera_position.y - VIDEO_SCREEN_H / 2);

    /* clip out */
    if(y >= VIDEO_SCREEN_H)
        return;

    /* adjust y */
    y = max(0, y);

    /* render */
    render_simple(y);
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
   y is given in screen space */
void render_simple(int y)
{
    v2d_t screen_size = video_get_screen_size();

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
    color_unmap(watercolor, &red, &green, &blue, &alpha);
    color_t color = color_premul_rgba(red, green, blue, alpha);

    /* render the water */
    image_rectfill(0, y, screen_size.x, screen_size.y, color);
}