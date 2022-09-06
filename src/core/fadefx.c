/*
 * Open Surge Engine
 * fadefx.c - fade effects
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

#include "fadefx.h"
#include "video.h"
#include "image.h"
#include "timer.h"
#include "util.h"

/* Fade-in & fade-out */
static enum { FADEFX_NONE, FADEFX_IN, FADEFX_OUT } type;
static bool just_ended;
static color_t fade_color;
static float elapsed_time;
static float total_time;

/*
 * fadefx_init()
 * Initializes the fade effect module
 */
void fadefx_init()
{
    type = FADEFX_NONE;
    just_ended = false;
    elapsed_time = 0.0f;
    total_time = 0.0f;
}

/*
 * fadefx_release()
 * Release the fade effect module
 */
void fadefx_release()
{
    ; /* empty */
}

/*
 * fadefx_update()
 * Updates the fade effect module
 */
void fadefx_update()
{
    just_ended = false;
    if(type != FADEFX_NONE) {
        uint8_t r, g, b;
        int alpha;

        /* elapsed time */
        elapsed_time += timer_get_delta();
        just_ended = (elapsed_time >= total_time);

        /* render */
        alpha = (int)(255.0f * (elapsed_time / (total_time * 0.8f)));
        alpha = clip(alpha, 0, 255);
        alpha = (type == FADEFX_IN) ? (255 - alpha) : alpha;
        color_unmap(fade_color, &r, &g, &b, NULL);
        image_rectfill(0, 0, VIDEO_SCREEN_W, VIDEO_SCREEN_H, color_rgba(r, g, b, alpha));
        
        /* the fade effect is over */
        if(just_ended) {
           total_time = elapsed_time = 0.0f;
           type = FADEFX_NONE;
        }
    }
}

/*
 * fadefx_in()
 * Fade-in effect
 */
void fadefx_in(color_t color, float seconds)
{
    if(type == FADEFX_NONE) {
        type = FADEFX_IN;
        just_ended = false;
        fade_color = color;
        elapsed_time = 0.0f;
        total_time = seconds;
    }
}

/*
 * fadefx_out()
 * Fade-out effect
 */
void fadefx_out(color_t color, float seconds)
{
    if(type == FADEFX_NONE) {
        type = FADEFX_OUT;
        just_ended = false;
        fade_color = color;
        elapsed_time = 0.0f;
        total_time = seconds;
    }
}

/*
 * fadefx_is_over()
 * Checks if the fade effect has just ended
 * ("only one action when this event loops")
 */
bool fadefx_is_over()
{
    return just_ended;
}

/*
 * fadefx_is_fading()
 * Is the fade effect ocurring?
 */
bool fadefx_is_fading()
{
    return (type != FADEFX_NONE);
}
