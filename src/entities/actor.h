/*
 * Open Surge Engine
 * actor.h - actor module
 * Copyright (C) 2008-2012, 2018-2019, 2021-2022  Alexandre Martins <alemartf@gmail.com>
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

#ifndef _ACTOR_H
#define _ACTOR_H

#include <stdbool.h>
#include "../core/v2d.h"
#include "../core/sprite.h"
#include "../core/input.h"

/* forward declarations */
struct brick_t;
struct brick_list_t;

/* actor structure */
typedef struct actor_t {

    /* movement data */
    v2d_t spawn_point;
    v2d_t position;
    v2d_t speed;
    float angle; /* angle = ang( actor's x-axis , real x-axis ), in radians */
    input_t *input; /* NULL by default (no input) */

    /* animation */
    const animation_t *animation;
    float animation_frame; /* controlled by a timer */
    float animation_speed_factor; /* default value: 1.0 */
    int synchronized_animation; /* synchronized animation? */
    int mirror; /* see the IF_* flags at image.h */
    int visible; /* is this actor visible? */
    float alpha; /* 0.0f (invisible) <= alpha <= 1.0f (opaque) */
    v2d_t hot_spot; /* anchor */
    v2d_t scale; /* scale */

} actor_t;


/* instantiation */
actor_t* actor_create();
void actor_destroy(actor_t *act);

/* rendering */
image_t* actor_image(const actor_t *act);
void actor_render(actor_t *act, v2d_t camera_position);
void actor_render_repeat_xy(actor_t *act, v2d_t camera_position, int repeat_x, int repeat_y);

/* animation */
void actor_change_animation(actor_t *act, const animation_t *anim);
void actor_change_animation_frame(actor_t *act, int frame);
void actor_change_animation_speed_factor(actor_t *act, float factor); /* default factor: 1.0 */
bool actor_animation_finished(const actor_t *act); /* true if the current animation has finished */
void actor_synchronize_animation(actor_t *act, int sync); /* should I use a shared animation frame? */
int actor_animation_frame(const actor_t* act);
v2d_t actor_action_spot(const actor_t* act); /* action spot appropriately flipped */

/* legacy */
int actor_collision(const actor_t *a, const actor_t *b); /* tests bounding-box collision between a and b */
int actor_brick_collision(const actor_t *act, const struct brick_t *brk); /* tests bounding-box collision with a brick */
void actor_sensors(actor_t *act, struct brick_list_t *brick_list, struct brick_t **up, struct brick_t **upright, struct brick_t **right, struct brick_t **downright, struct brick_t **down, struct brick_t **downleft, struct brick_t **left, struct brick_t **upleft); /* get obstacle bricks around the actor */
const struct brick_t* actor_brick_at(actor_t *act, const struct brick_list_t *brick_list, v2d_t offset);


#endif
