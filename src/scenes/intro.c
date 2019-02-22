/*
 * Open Surge Engine
 * intro.c - introduction screen
 * Copyright (C) 2008-2011, 2013, 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include <stdlib.h>
#include "intro.h"
#include "../core/v2d.h"
#include "../core/timer.h"
#include "../core/scene.h"
#include "../core/storyboard.h"
#include "../core/fadefx.h"
#include "../core/image.h"
#include "../core/video.h"
#include "../core/color.h"
#include "../core/audio.h"
#include "../core/input.h"
#include "../core/font.h"
#include "../core/soundfactory.h"

/* private data */
#define INTRO_TIMEOUT       3.0f
#define INTRO_FADETIME      0.5f
#define INTRO_BGCOLOR()     color_hex("0a0a0a")
#define INTRO_FONT          "GoodNeighbors"
#define INTRO_TEXT          "powered by\nOpen Surge Engine"
static float elapsed_time;
static int debug_mode;
static font_t* fnt;
static input_t* in;

/* public functions */

/*
 * intro_init()
 * Initializes the introduction scene
 */
void intro_init(void *foo)
{
    /* initialize variables */
    elapsed_time = 0.0f;
    debug_mode = FALSE;
    in = input_create_user(NULL);

    /* create font */
    fnt = font_create(INTRO_FONT);
    font_set_text(fnt, INTRO_TEXT);
    font_set_align(fnt, FONTALIGN_CENTER);
    
    /* position font */
    font_set_position(fnt, v2d_add(
        v2d_multiply(video_get_screen_size(), 0.5f),
        v2d_new(0, -font_get_textsize(fnt).y * 0.5f)
    ));

    /* misc */
    fadefx_in(color_rgb(0,0,0), INTRO_FADETIME);
    music_stop();
}

/*
 * intro_release()
 * Releases the introduction scene
 */
void intro_release()
{
    font_destroy(fnt);
    input_destroy(in);
}

/*
 * intro_update()
 * Updates the introduction scene
 */
void intro_update()
{
    /* elapsed time */
    elapsed_time += timer_get_delta();

    /* skip scene */
    if(!fadefx_is_fading() && (input_button_pressed(in, IB_FIRE1) || input_button_pressed(in, IB_FIRE3) || input_button_pressed(in, IB_FIRE4)))
        elapsed_time += INTRO_TIMEOUT;

    /* done */
    if(elapsed_time >= INTRO_TIMEOUT) {
        if(fadefx_over()) {
            scenestack_pop();
            if(debug_mode)
                scenestack_push(storyboard_get_scene(SCENE_STAGESELECT), &debug_mode);
            return;
        }
        fadefx_out(color_rgb(0,0,0), INTRO_FADETIME);
    }

    /* secret */
    if(input_button_pressed(in, IB_RIGHT)) {
        static int cnt = 0;
        if(!debug_mode && ++cnt == 3) {
            sound_play(SFX_SECRET);
            elapsed_time += INTRO_TIMEOUT;
            debug_mode = TRUE;
            cnt = 0;
        }
    }
}

/*
 * intro_render()
 * Renders the introduction scene
 */
void intro_render()
{
    v2d_t camera = v2d_multiply(video_get_screen_size(), 0.5f);
    image_clear(INTRO_BGCOLOR());
    font_render(fnt, camera);
}