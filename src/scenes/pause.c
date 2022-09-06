/*
 * Open Surge Engine
 * pause.c - "pause" scene
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

#include <stdbool.h>
#include <math.h>
#include "pause.h"
#include "confirmbox.h"
#include "quest.h"
#include "../core/scene.h"
#include "../core/storyboard.h"
#include "../core/util.h"
#include "../core/fadefx.h"
#include "../core/color.h"
#include "../core/video.h"
#include "../core/input.h"
#include "../core/lang.h"
#include "../core/sprite.h"
#include "../core/audio.h"
#include "../core/timer.h"
#include "../scripting/scripting.h"


/* private data */
static image_t *pause_buf;
static input_t *pause_input;
static bool pause_ready, pause_quit;
static float pause_timer;


/*
 * pause_init()
 * Initializes the pause screen
 */
void pause_init(void *foo)
{
    pause_input = input_create_user(NULL);
    pause_buf = image_clone(video_get_backbuffer());
    pause_ready = false;
    pause_quit = false;
    pause_timer = 0.0f;

    music_pause();
    scripting_pause_vm();
}


/*
 * pause_release()
 * Releases the pause screen
 */
void pause_release()
{
    scripting_resume_vm();
    music_resume();

    image_destroy(pause_buf);
    input_destroy(pause_input);
}


/*
 * pause_update()
 * Updates the pause screen
 */
void pause_update()
{
    /* update timer */
    if(!pause_quit)
        pause_timer += timer_get_delta();

    /* quit */
    if(input_button_pressed(pause_input, IB_FIRE4)) {
        confirmboxdata_t cbd = { "$QUIT_QUESTION", "$QUIT_OPTION1", "$QUIT_OPTION2", 2 };
        scenestack_push(storyboard_get_scene(SCENE_CONFIRMBOX), &cbd);
        return;
    }

    if(1 == confirmbox_selected_option())
        pause_quit = true;

    if(pause_quit) {
        if(fadefx_is_over()) {
            scenestack_pop();
            scenestack_pop();
            quest_abort();
            return;
        }
        fadefx_out(color_rgb(0,0,0), 1.0f);
        return;
    }

    /* unpause */
    if(pause_ready) {
        if(input_button_pressed(pause_input, IB_FIRE3)) {
            scenestack_pop();
            return;
        }
    }
    else {
        if(input_button_up(pause_input, IB_FIRE3))
            pause_ready = true;
    }
}


/* 
 * pause_render()
 * Renders the pause screen
 */
void pause_render()
{
    image_t *p = sprite_get_image(sprite_get_animation("Pause", 0), 0);
    v2d_t size = v2d_new(image_width(p), image_height(p));

    const float frequency = 1.5707963267948966f; /* PI / 2 */
    float scale = 1.0f + 0.5f * fabs(cosf(frequency * pause_timer));

    v2d_t pos = v2d_new(
        (VIDEO_SCREEN_W - size.x) / 2.0f - (scale - 1.0f) * size.x / 2.0f,
        (VIDEO_SCREEN_H - size.y) / 2.0f - (scale - 1.0f) * size.y / 2.0f
    );

    image_blit(pause_buf, 0, 0, 0, 0, image_width(pause_buf), image_height(pause_buf));
    image_draw_scaled(p, (int)pos.x, (int)pos.y, v2d_new(scale,scale), IF_NONE);
}