/*
 * Open Surge Engine
 * fadefx.c - fade effects
 * Copyright (C) 2009, 2013  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
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

#include <allegro.h>
#include "fadefx.h"
#include "video.h"
#include "timer.h"
#include "util.h"

#define IMAGE2BITMAP(img)       (*((BITMAP**)(img)))   /* whoooa, this is crazy stuff */

/* Fade-in & fade-out */
static enum { FADEFX_NONE, FADEFX_IN, FADEFX_OUT } type;
static int end;
static uint32 fade_color;
static float elapsed_time;
static float total_time;

/*
 * fadefx_init()
 * Initializes the fade effect module
 */
void fadefx_init()
{
    type = FADEFX_NONE;
    end = FALSE;
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
    int n, gambiarra = FALSE;

    /* logic */
    end = FALSE;
    if(type != FADEFX_NONE) {
        elapsed_time += timer_get_delta();
        if(elapsed_time >= total_time) { /* the fade effect is over */
            end = TRUE;
            gambiarra = (type == FADEFX_OUT);
            type = FADEFX_NONE;
            total_time = elapsed_time = 0.0f;
        }
    }

    /* render */
    if(type != FADEFX_NONE) {
        n = (int)( 255.0f * ((elapsed_time * 1.25f) / total_time) );
        n = clip(n, 0, 255);
        n = (type == FADEFX_IN) ? (255 - n) : n;
        drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
        set_trans_blender(0, 0, 0, n);
        rectfill(IMAGE2BITMAP(video_get_backbuffer()), 0, 0, VIDEO_SCREEN_W, VIDEO_SCREEN_H, fade_color);
        solid_mode();
    }
    else if(end && gambiarra)
        rectfill(IMAGE2BITMAP(video_get_backbuffer()), 0, 0, VIDEO_SCREEN_W, VIDEO_SCREEN_H, fade_color);
}

/*
 * fadefx_in()
 * Fade-in effect
 */
void fadefx_in(uint32 color, float seconds)
{
    if(type == FADEFX_NONE) {
        type = FADEFX_IN;
        end = FALSE;
        fade_color = color;
        elapsed_time = 0;
        total_time = seconds;
    }
}

/*
 * fadefx_out()
 * Fade-out effect
 */
void fadefx_out(uint32 color, float seconds)
{
    if(type == FADEFX_NONE) {
        type = FADEFX_OUT;
        end = FALSE;
        fade_color = color;
        elapsed_time = 0;
        total_time = seconds;
    }
}

/*
 * fadefx_over()
 * Asks if the fade effect has ended
 * ("only one action when this event loops")
 */
int fadefx_over()
{
    return end;
}

/*
 * fadefx_is_fading()
 * Is the fade effect ocurring?
 */
int fadefx_is_fading()
{
    return (type != FADEFX_NONE);
}
