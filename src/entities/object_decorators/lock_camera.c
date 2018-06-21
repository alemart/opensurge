/*
 * Open Surge Engine
 * lock_camera.c - Locks an area of the playfield
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

#include <math.h>
#include "lock_camera.h"
#include "../../core/util.h"
#include "../../core/image.h"
#include "../../core/video.h"
#include "../../scenes/level.h"
#include "../../entities/player.h"

/* objectdecorator_lockcamera_t class */
typedef struct objectdecorator_lockcamera_t objectdecorator_lockcamera_t;
struct objectdecorator_lockcamera_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *x1, *y1, *x2, *y2;
    int has_locked_somebody;
    int _x1, _y1, _x2, _y2;
};

/* private methods */
static void lockcamera_init(objectmachine_t *obj);
static void lockcamera_release(objectmachine_t *obj);
static void lockcamera_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void lockcamera_render(objectmachine_t *obj, v2d_t camera_position);

static void get_rectangle_coordinates(objectdecorator_lockcamera_t *me, int *x1, int *y1, int *x2, int *y2);
static void update_rectangle_coordinates(objectdecorator_lockcamera_t *me, int x1, int y1, int x2, int y2);


/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_lockcamera_new(objectmachine_t *decorated_machine, expression_t *x1, expression_t *y1, expression_t *x2, expression_t *y2)
{
    objectdecorator_lockcamera_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = lockcamera_init;
    obj->release = lockcamera_release;
    obj->update = lockcamera_update;
    obj->render = lockcamera_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    me->x1 = x1;
    me->y1 = y1;
    me->x2 = x2;
    me->y2 = y2;

    return obj;
}





/* private methods */
void lockcamera_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_lockcamera_t *me = (objectdecorator_lockcamera_t*)obj;
    int x1, x2, y1, y2;

    me->has_locked_somebody = FALSE;
    get_rectangle_coordinates(me, &x1, &y1, &x2, &y2);
    update_rectangle_coordinates(me, x1, y1, x2, y2);

    decorated_machine->init(decorated_machine);
}

void lockcamera_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_lockcamera_t *me = (objectdecorator_lockcamera_t*)obj;
    player_t *player = enemy_get_observed_player(obj->get_object_instance(obj));

    if(me->has_locked_somebody) {
        player->in_locked_area = FALSE;
        level_unlock_camera();
    }

    expression_destroy(me->x1);
    expression_destroy(me->x2);
    expression_destroy(me->y1);
    expression_destroy(me->y2);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void lockcamera_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    object_t *object = obj->get_object_instance(obj);
    player_t *player = enemy_get_observed_player(object);
    objectdecorator_lockcamera_t *me = (objectdecorator_lockcamera_t*)obj;
    actor_t *act = object->actor, *ta;
    float rx, ry, rw, rh;
    int x1, x2, y1, y2;
    int i;

    /* configuring the rectangle */
    get_rectangle_coordinates(me, &x1, &y1, &x2, &y2);
    update_rectangle_coordinates(me, x1, y1, x2, y2);

    /* my rectangle, in world coordinates */
    rx = act->position.x + x1;
    ry = act->position.y + y1;
    rw = x2 - x1;
    rh = y2 - y1;

    /* only the observed player can enter this area */
    for(i=0; i<team_size; i++) {
        ta = team[i]->actor;

        if(team[i] != player) {
            /* hey, you can't enter here! */
            float border = 30.0f;
            if(ta->position.x > rx - border && ta->position.x < rx) {
                ta->position.x = rx - border;
                ta->speed.x = 0.0f;
            }
            if(ta->position.x > rx + rw && ta->position.x < rx + rw + border) {
                ta->position.x = rx + rw + border;
                ta->speed.x = 0.0f;
            }
        }
        else {
            /* test if the player has got inside my rectangle */
            float a[4], b[4];

            a[0] = ta->position.x;
            a[1] = ta->position.y;
            a[2] = ta->position.x + 1;
            a[3] = ta->position.y + 1;

            b[0] = rx;
            b[1] = ry;
            b[2] = rx + rw;
            b[3] = ry + rh;

            if(bounding_box(a, b)) {
                /* welcome, player! You have been locked. BWHAHAHA!!! */
                me->has_locked_somebody = TRUE;
                team[i]->in_locked_area = TRUE;
                level_lock_camera(rx, ry, rx+rw, ry+rh);
            }
        }
    }


    /* cage */
    if(me->has_locked_somebody) {
        ta = player->actor;
        if(ta->position.x < rx) {
            ta->position.x = rx;
            ta->speed.x = max(0.0f, ta->speed.x);
            player->at_some_border = TRUE;
        }
        if(ta->position.x > rx + rw) {
            ta->position.x = rx + rw;
            ta->speed.x = min(0.0f, ta->speed.x);
            player->at_some_border = TRUE;
        }
        ta->position.y = clip(ta->position.y, ry, ry + rh);
    }

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void lockcamera_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    if(level_editmode()) {
        objectdecorator_lockcamera_t *me = (objectdecorator_lockcamera_t*)obj;
        actor_t *act = obj->get_object_instance(obj)->actor;
        uint32 color = image_rgb(255, 0, 0);
        int x1, y1, x2, y2;

        x1 = (act->position.x + me->_x1) - (camera_position.x - VIDEO_SCREEN_W/2);
        y1 = (act->position.y + me->_y1) - (camera_position.y - VIDEO_SCREEN_H/2);
        x2 = (act->position.x + me->_x2) - (camera_position.x - VIDEO_SCREEN_W/2);
        y2 = (act->position.y + me->_y2) - (camera_position.y - VIDEO_SCREEN_H/2);

        image_line(video_get_backbuffer(), x1, y1, x2, y1, color);
        image_line(video_get_backbuffer(), x2, y1, x2, y2, color);
        image_line(video_get_backbuffer(), x2, y2, x1, y2, color);
        image_line(video_get_backbuffer(), x1, y2, x1, y1, color);
    }

    decorated_machine->render(decorated_machine, camera_position);
}


/* auxiliary functions */

void get_rectangle_coordinates(objectdecorator_lockcamera_t *me, int *x1, int *y1, int *x2, int *y2)
{
    int mi, ma;

    *x1 = (int)expression_evaluate(me->x1);
    *x2 = (int)expression_evaluate(me->x2);
    *y1 = (int)expression_evaluate(me->y1);
    *y2 = (int)expression_evaluate(me->y2);

    if(*x1 == *x2) (*x2)++;
    if(*y1 == *y2) (*y2)++;

    mi = min(*x1, *x2);
    ma = max(*x1, *x2);
    *x1 = mi;
    *x2 = ma;

    mi = min(*y1, *y2);
    ma = max(*y1, *y2);
    *y1 = mi;
    *y2 = ma;
}

void update_rectangle_coordinates(objectdecorator_lockcamera_t *me, int x1, int y1, int x2, int y2)
{
    me->_x1 = x1;
    me->_y1 = y1;
    me->_x2 = x2;
    me->_y2 = y2;
}
