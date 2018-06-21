/*
 * Open Surge Engine
 * gravity.c - This decorator makes the object capable of being affected by gravity
 * Copyright (C) 2010, 2011  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include "gravity.h"
#include "../../core/util.h"
#include "../../core/timer.h"
#include "../../core/video.h"
#include "../../scenes/level.h"

/* objectdecorator_gravity_t class */
typedef struct objectdecorator_gravity_t objectdecorator_gravity_t;
struct objectdecorator_gravity_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
};

/* private methods */
static void gravity_init(objectmachine_t *obj);
static void gravity_release(objectmachine_t *obj);
static void gravity_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void gravity_render(objectmachine_t *obj, v2d_t camera_position);

static int hit_test(int x, int y, const image_t *brk_image, int brk_x, int brk_y);
static int sticky_test(const actor_t *act, const brick_list_t *brick_list);


/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_gravity_new(objectmachine_t *decorated_machine)
{
    objectdecorator_gravity_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = gravity_init;
    obj->release = gravity_release;
    obj->update = gravity_update;
    obj->render = gravity_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    return obj;
}




/* private methods */
void gravity_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void gravity_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void gravity_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    object_t *object = obj->get_object_instance(obj);
    actor_t *act = object->actor;
    float dt = timer_get_delta();

    /* --------------------------- */

    /* in order to avoid too much processor load,
       we adopt this simplified platform system */
    int rx, ry, rw, rh, bx, by, bw, bh, j;
    const image_t *ri, *bi;
    brick_list_t *it;
    enum { NONE, FLOOR, CEILING } collided = NONE;
    int i, sticky_max_offset = 3;

    ri = actor_image(act);
    rx = (int)(act->position.x - act->hot_spot.x);
    ry = (int)(act->position.y - act->hot_spot.y);
    rw = image_width(ri);
    rh = image_height(ri);

    /* check for collisions */
    for(it = brick_list; it != NULL && collided == NONE; it = it->next) {
        if(it->data->brick_ref->property != BRK_NONE) {
            bi = it->data->brick_ref->image;
            bx = it->data->x;
            by = it->data->y;
            bw = image_width(bi);
            bh = image_height(bi);

            if(rx<bx+bw && rx+rw>bx && ry<by+bh && ry+rh>by) {
                if(image_pixelperfect_collision(ri, bi, rx, ry, bx, by)) {
                    if(hit_test(rx+rw/2, ry, bi, bx, by)) {
                        /* ceiling */
                        collided = CEILING;
                        for(j=1; j<=bh; j++) {
                            if(!image_pixelperfect_collision(ri, bi, rx, ry+j, bx, by)) {
                                act->position.y += j-1;
                                break;
                            }
                        }
                    }
                    else if(hit_test(rx+rw/2, ry+rh-1, bi, bx, by)) {
                        /* floor */
                        collided = FLOOR;
                        for(j=1; j<=bh && hit_test(rx+rw/2, ry+rh-j, bi, bx, by); j++)
                            act->position.y -= 1;
                        if(j > 1) act->position.y += 1;
                    }
                }
            }
        }
    }

    /* collided & gravity */
    switch(collided) {
        case FLOOR:
            if(act->speed.y > 0.0f)
                act->speed.y = 0.0f;
            break;

        case CEILING:
            if(act->speed.y < 0.0f)
                act->speed.y = 0.0f;
            break;

        default:
            act->speed.y += (0.21875f * 60.0f * 60.0f) * dt;
            break;
    }

    /* move */
    act->position.y += act->speed.y * dt;

    /* sticky physics */
    if(!sticky_test(act, brick_list)) {
        for(i=sticky_max_offset; i>0; i--) {
            act->position.y += i;
            if(!sticky_test(act, brick_list)) {
                act->position.y += (i == sticky_max_offset) ? -i : 1;
                break;
            }
            else
                act->position.y -= i;
        }
    }

    /* --------------------------- */

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void gravity_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}




/* (x,y) collides with the brick */
int hit_test(int x, int y, const image_t *brk_image, int brk_x, int brk_y)
{
    if(x >= brk_x && x < brk_x + image_width(brk_image) && y >= brk_y && y < brk_y + image_height(brk_image))
        return image_getpixel(brk_image, x - brk_x, y - brk_y) != video_get_maskcolor();

    return FALSE;
}

/* act collides with some brick? */
int sticky_test(const actor_t *act, const brick_list_t *brick_list)
{
    const brick_list_t *it;
    const brick_t *b;
    const image_t *ri;
    int rx, ry, rw, rh;

    ri = actor_image(act);
    rx = (int)(act->position.x - act->hot_spot.x);
    ry = (int)(act->position.y - act->hot_spot.y);
    rw = image_width(ri);
    rh = image_height(ri);

    for(it = brick_list; it; it = it->next) {
        b = it->data;
        if(b->brick_ref->property != BRK_NONE) {
            if(hit_test(rx+rw/2, ry+rh-1, brick_image(b), b->x, b->y))
                return TRUE;
        }
    }

    return FALSE;
}
