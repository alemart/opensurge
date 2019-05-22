/*
 * Open Surge Engine
 * door.c - door
 * Copyright (C) 2010  Alexandre Martins <alemartf@gmail.com>
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

#include "door.h"
#include "../../core/util.h"
#include "../../core/audio.h"
#include "../../core/timer.h"
#include "../../core/soundfactory.h"
#include "../player.h"
#include "../brick.h"
#include "../item.h"
#include "../enemy.h"
#include "../actor.h"

/* door class */
typedef struct door_t door_t;
struct door_t {
    item_t item; /* base class */
    int is_closed; /* is the door closed? */
};

static void door_init(item_t *item);
static void door_release(item_t* item);
static void door_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void door_render(item_t* item, v2d_t camera_position);



/* public methods */
item_t* door_create()
{
    item_t *item = mallocx(sizeof(door_t));

    item->init = door_init;
    item->release = door_release;
    item->update = door_update;
    item->render = door_render;

    return item;
}

void door_open(item_t *door)
{
    door_t *me = (door_t*)door;
    me->is_closed = FALSE;
    sound_play( soundfactory_get("open door") );
}

void door_close(item_t *door)
{
    door_t *me = (door_t*)door;
    me->is_closed = TRUE;
    sound_play( soundfactory_get("close door") );
}


/* private methods */
void door_init(item_t *item)
{
    door_t *me = (door_t*)item;

    item->always_active = TRUE;
    item->obstacle = TRUE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    me->is_closed = TRUE;
    actor_change_animation(item->actor, sprite_get_animation("SD_DOOR", 0));
}



void door_release(item_t* item)
{
    actor_destroy(item->actor);
}



void door_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    door_t *me = (door_t*)item;
    actor_t *act = item->actor;
    float speed = 2000.0f;
    float dt = timer_get_delta();

    if(me->is_closed)
        act->position.y = min(act->position.y + speed*dt, act->spawn_point.y);
    else    
        act->position.y = max(act->position.y - speed*dt, act->spawn_point.y - image_height(actor_image(act)) * 0.8);
}


void door_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}

