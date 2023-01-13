/*
 * Open Surge Engine
 * popup.c - popup scene for mobile devices
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
#include "popup.h"
#include "subscenes/subscene.h"
#include "util/touch.h"
#include "../../core/scene.h"
#include "../../core/timer.h"
#include "../../core/util.h"
#include "../../core/video.h"
#include "../../core/image.h"
#include "../../core/input.h"
#include "../../core/logfile.h"
#include "../../entities/actor.h"


/* touch/mouse input */
#define LOG(...)        logfile_message("Mobile Popup - " __VA_ARGS__)
static const inputbutton_t BACK_BUTTON = IB_FIRE4;
static input_t* input = NULL;
static input_t* mouse_input = NULL;
static void on_touch_start(v2d_t touch_start);
static void on_touch_end(v2d_t touch_start, v2d_t touch_end);
static void on_touch_move(v2d_t touch_start, v2d_t touch_current);


/* swipe logic */
#define SWIPE_DOWN_MINDIST (VIDEO_SCREEN_H / 4)
static const float SWIPE_DOWN_ANGLE = 0.866f; /* sin(60 deg) */
static const float TRANSITION_TIME = 0.25f;
static v2d_t scroll = (v2d_t){ .x = 0.0f, .y = 0.0f };


/* state */
static enum { OPENING, WAITING, CLOSING } state = OPENING;
static void update_opening();
static void update_waiting();
static void update_closing();
static void (*update[])() = {
    [OPENING] = update_opening,
    [WAITING] = update_waiting,
    [CLOSING] = update_closing
};


/* private */
static image_t* background = NULL;
static mobile_subscene_t* subscene = NULL;



/*
 * mobilepopup_init()
 * Initializes the mobile popup
 */
void mobilepopup_init(void *subscene_ptr)
{
    LOG("Opening");

    background = image_clone(video_get_backbuffer());
    mouse_input = input_create_mouse();
    input = input_create_user(NULL);

    state = OPENING;
    scroll = v2d_new(0, VIDEO_SCREEN_H);

    subscene = (mobile_subscene_t*)subscene_ptr;
    subscene->init(subscene);
}



/*
 * mobilepopup_release()
 * Releases the mobile popup
 */
void mobilepopup_release()
{
    LOG("Closing");

    subscene->release(subscene);

    input_destroy(input);
    input_destroy(mouse_input);
    image_destroy(background);
}



/*
 * mobilepopup_update()
 * Updates the mobile popup
 */
void mobilepopup_update()
{
    /* update the subscene */
    subscene->update(subscene, scroll);
    /*if(changed_scene()) return;*/

    /* check if the back button has been pressed on the smartphone */
    if(input_button_pressed(input, BACK_BUTTON))
        state = CLOSING;

    /* update the popup */
    update[state]();
    /*if(changed_scene()) return;*/
}



/*
 * mobilepopup_render()
 * Renders the mobile popup
 */
void mobilepopup_render()
{
    /* render the background transition */
    float dy = VIDEO_SCREEN_H - scroll.y;
    image_blit(background, 0, 0, 0, -dy, image_width(background), image_height(background));

    /* render the subscene */
    subscene->render(subscene, scroll);
}



/* private stuff */

void update_opening()
{
    float dt = timer_get_delta();
    float v = VIDEO_SCREEN_H / TRANSITION_TIME;

    scroll.y -= v * dt;
    if(scroll.y <= 0.0f) {
        scroll.y = 0.0f;
        state = WAITING;
    }
}

void update_closing()
{
    float dt = timer_get_delta();
    float v = VIDEO_SCREEN_H / TRANSITION_TIME;

    scroll.y += v * dt;
    if(scroll.y >= VIDEO_SCREEN_H) {
        scroll.y = VIDEO_SCREEN_H;
        scenestack_pop();
        return;
    }
}

void update_waiting()
{
    handle_touch_input(mouse_input, on_touch_start, on_touch_end, on_touch_move);
}

void on_touch_start(v2d_t touch_start)
{
    ;
}

void on_touch_end(v2d_t touch_start, v2d_t touch_end)
{
    v2d_t ds = v2d_subtract(touch_end, touch_start);
    float length = v2d_magnitude(ds);
    v2d_t direction = v2d_normalize(ds);

    state = OPENING;
    if(direction.y >= SWIPE_DOWN_ANGLE) {
        if(length >= SWIPE_DOWN_MINDIST)
            state = CLOSING;
    }
}

void on_touch_move(v2d_t touch_start, v2d_t touch_current)
{
    v2d_t ds = v2d_subtract(touch_current, touch_start);

    scroll.y = max(0.0f, ds.y);
}