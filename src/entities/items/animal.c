/*
 * Open Surge Engine
 * animal.c - little animal
 * Copyright (C) 2010, 2011, 2019  Alexandre Martins <alemartf@gmail.com>
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

#include "animal.h"
#include "../../core/util.h"
#include "../../core/timer.h"
#include "../../core/video.h"
#include "../../scenes/level.h"
#include "../player.h"
#include "../brick.h"
#include "../item.h"
#include "../enemy.h"
#include "../actor.h"
#include "../../physics/obstacle.h"


/* animal class */
typedef struct animal_t animal_t;
struct animal_t {
    item_t item; /* base class */
    int animal_id;
    int is_running;
};

static void animal_init(item_t *item);
static void animal_release(item_t* item);
static void animal_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void animal_render(item_t* item, v2d_t camera_position);

/* public methods */
item_t* animal_create()
{
    item_t *item = mallocx(sizeof(animal_t));

    item->init = animal_init;
    item->release = animal_release;
    item->update = animal_update;
    item->render = animal_render;

    return item;
}


/* private methods */
void animal_init(item_t *item)
{
    const int MAX_ANIMALS = 12;
    animal_t *me = (animal_t*)item;

    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = FALSE;
    item->actor = actor_create();
    item->actor->speed.x = (random(2) ? 1 : -1) * (45 + random(21));

    me->is_running = FALSE;
    me->animal_id = random(MAX_ANIMALS);
    actor_change_animation(item->actor, sprite_get_animation("SD_ANIMAL", 0));
}



void animal_release(item_t* item)
{
    actor_destroy(item->actor);
}



void animal_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    float dt = timer_get_delta();
    animal_t *me = (animal_t*)item;
    actor_t *act = item->actor;
    int animation_id = 2*me->animal_id + (me->is_running?1:0);

    /* in order to avoid too much processor load,
       we adopt this simplified platform system */
    int rx, ry, rw, rh, bx, by, bw, bh, j;
    const image_t *ri;
    brick_list_t *it;
    enum { NONE, FLOOR, RIGHTWALL, CEILING, LEFTWALL } bounce = NONE;
    const obstacle_t* bo;

    ri = actor_image(act);
    rx = (int)(act->position.x - act->hot_spot.x);
    ry = (int)(act->position.y - act->hot_spot.y);
    rw = image_width(ri);
    rh = image_height(ri);

    /* check for collisions */
    for(it = brick_list; it != NULL && bounce == NONE; it = it->next) {
        bo = brick_obstacle(it->data);
        if(bo && brick_type(it->data) != BRK_PASSABLE) {
            bx = brick_position(it->data).x;
            by = brick_position(it->data).y;
            bw = brick_size(it->data).x;
            bh = brick_size(it->data).y;

            if(rx<bx+bw && rx+rw>bx && ry<by+bh && ry+rh>by) {
                if(obstacle_got_collision(bo, rx, ry+rh/2, rx, ry+rh/2)) {
                    /* left wall */
                    bounce = LEFTWALL;
                    for(j=1; j<=bw; j++) {
                        if(!obstacle_got_collision(bo, rx+j, ry, rx+j, ry)) {
                            act->position.x += j-1;
                            break;
                        }
                    }
                }
                else if(obstacle_got_collision(bo, rx+rw-1, ry+rh/2, rx+rw-1, ry+rh/2)) {
                    /* right wall */
                    bounce = RIGHTWALL;
                    for(j=1; j<=bw; j++) {
                        if(!obstacle_got_collision(bo, rx-j, ry, rx-j, ry)) {
                            act->position.x -= j-1;
                            break;
                        }
                    }
                }
                else if(obstacle_got_collision(bo, rx+rw/2, ry, rx+rw/2, ry)) {
                    /* ceiling */
                    bounce = CEILING;
                    for(j=1; j<=bh; j++) {
                        if(!obstacle_got_collision(bo, rx, ry+j, rx, ry+j)) {
                            act->position.y += j-1;
                            break;
                        }
                    }
                }
                else if(obstacle_got_collision(bo, rx+rw/2, ry+rh-1, rx+rw/2, ry+rh-1)) {
                    /* floor */
                    bounce = FLOOR;
                    for(j=1; j<=bh; j++) {
                        if(!obstacle_got_collision(bo, rx, ry-j, rx, ry-j)) {
                            act->position.y -= j-1;
                            break;
                        }
                    }
                }
            }
        }
    }

    /* bounce & gravity */
    switch(bounce) {
        case FLOOR:
            me->is_running = TRUE;
            if(act->speed.y > 0.0f)
                act->speed.y = -240.0f-random(27);
            break;

        case RIGHTWALL:
            if(act->speed.x > 0.0f)
                act->speed.x *= -1.0f;
            break;

        case LEFTWALL:
            if(act->speed.x < 0.0f)
                act->speed.x *= -1.0f;
            break;

        case CEILING:
            if(act->speed.y < 0.0f)
                act->speed.y *= -0.25f;
            break;

        default:
            act->speed.y += (0.21875f * 60.0f * 60.0f) * dt;
            break;
    }

    /* move */
    if(me->is_running)
        act->position.x += act->speed.x * dt;
    act->position.y += act->speed.y * dt;

    /* animation */
    act->mirror = (act->speed.x >= 0.0f) ? IF_NONE : IF_HFLIP;
    actor_change_animation(item->actor, sprite_get_animation("SD_ANIMAL", animation_id));
}


void animal_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}