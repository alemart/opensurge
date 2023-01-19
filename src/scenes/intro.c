/*
 * Open Surge Engine
 * intro.c - introduction screen
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

#include <stdbool.h>
#include <stdlib.h>
#include "intro.h"
#include "../core/global.h"
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
#include "../core/mobile_gamepad.h"
#include "../entities/sfx.h"

/* private data */
#define INTRO_TIMEOUT       3.0f
#define INTRO_FADETIME      0.5f
#define INTRO_FONT          "GoodNeighbors"
#define INTRO_TEXT          "Open Surge Engine\nopensurge2d.org"
#define PRIMARY_COLOR       "424c6e"
#define SECONDARY_COLOR     "657392"
static float elapsed_time;
static bool debug_mode;
static font_t* fnt;
static input_t* in;
static image_t* box;
static bool any_button_pressed(input_t* in);


/* public functions */

/*
 * intro_init()
 * Initializes the introduction scene
 */
void intro_init(void *foo)
{
    image_t* target;

    /* initialize variables */
    elapsed_time = 0.0f;
    debug_mode = false;
    in = input_create_user(NULL);

    /* create box */
    box = image_create(VIDEO_SCREEN_W * 3 / 2, VIDEO_SCREEN_H * 9 / 10);
    target = image_drawing_target();
    image_set_drawing_target(box);
    image_clear(color_hex(SECONDARY_COLOR));
    image_set_drawing_target(target);

    /* create font */
    fnt = font_create(INTRO_FONT);
    font_set_text(fnt, "%s", INTRO_TEXT);
    font_set_align(fnt, FONTALIGN_CENTER);
    
    /* position font */
    font_set_position(fnt, v2d_add(
        v2d_multiply(video_get_screen_size(), 0.5f),
        v2d_new(0, -font_get_textsize(fnt).y * 0.5f)
    ));

    /* misc */
    fadefx_in(color_rgb(0,0,0), INTRO_FADETIME);
    music_stop();
    mobilegamepad_fadeout();
}

/*
 * intro_release()
 * Releases the introduction scene
 */
void intro_release()
{
    mobilegamepad_fadein();
    font_destroy(fnt);
    image_destroy(box);
    input_destroy(in);
}

/*
 * intro_update()
 * Updates the introduction scene
 */
void intro_update()
{
    static int cnt = 0;

    /* elapsed time */
    elapsed_time += timer_get_delta();

    /* skip scene */
    if(!fadefx_is_fading() && (input_button_pressed(in, IB_FIRE1) || input_button_pressed(in, IB_FIRE3) || input_button_pressed(in, IB_FIRE4)))
        elapsed_time += INTRO_TIMEOUT;

    /* done */
    if(elapsed_time >= INTRO_TIMEOUT) {
        if(fadefx_is_over()) {
            scenestack_pop();
            if(debug_mode)
                scenestack_push(storyboard_get_scene(SCENE_STAGESELECT), &debug_mode);
            return;
        }
        fadefx_out(color_rgb(0,0,0), INTRO_FADETIME);
    }

    /* secret */
    if(input_button_pressed(in, IB_RIGHT)) {
        if(!debug_mode && ++cnt == 3) {
            sound_play(SFX_SECRET);
            elapsed_time += INTRO_TIMEOUT;
            debug_mode = true;
            cnt = 0;
        }
    }
    else if(any_button_pressed(in) && cnt < 3)
        cnt = 0;
}

/*
 * intro_render()
 * Renders the introduction scene
 */
void intro_render()
{
    v2d_t camera = v2d_multiply(video_get_screen_size(), 0.5f);
    const float angle = 18.45f / 57.2957795131f;

    image_clear(color_hex(PRIMARY_COLOR));
    image_draw_rotated(box, VIDEO_SCREEN_W / 2, VIDEO_SCREEN_H / 2, image_width(box)/2, image_height(box)/2, angle, IF_NONE);
    font_render(fnt, camera);
}



/* private stuff */

/* checks if any button has been pressed */
bool any_button_pressed(input_t* in)
{
    for(int i = 0; i < (int)IB_MAX; i++) {
        if(input_button_pressed(in, (inputbutton_t)i))
            return true;
    }

    return false;
}