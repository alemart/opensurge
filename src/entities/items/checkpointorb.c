/*
 * Open Surge Engine
 * checkpointorb.c - checkpoint orb
 * Copyright (C) 2010  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include "checkpointorb.h"
#include "../../core/util.h"
#include "../../core/soundfactory.h"
#include "../../scenes/level.h"
#include "../player.h"
#include "../brick.h"
#include "../item.h"
#include "../enemy.h"
#include "../actor.h"

/* checkpointorb class */
typedef struct checkpointorb_t checkpointorb_t;
struct checkpointorb_t {
    item_t item; /* base class */
    int is_active; /* has this checkpoint orb been touched? */
};

static void checkpointorb_init(item_t *item);
static void checkpointorb_release(item_t* item);
static void checkpointorb_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void checkpointorb_render(item_t* item, v2d_t camera_position);



/* public methods */
item_t* checkpointorb_create()
{
    item_t *item = mallocx(sizeof(checkpointorb_t));

    item->init = checkpointorb_init;
    item->release = checkpointorb_release;
    item->update = checkpointorb_update;
    item->render = checkpointorb_render;

    return item;
}


/* private methods */
void checkpointorb_init(item_t *item)
{
    checkpointorb_t *me = (checkpointorb_t*)item;

    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    me->is_active = FALSE;
    actor_change_animation(item->actor, sprite_get_animation("SD_CHECKPOINT", 0));
}



void checkpointorb_release(item_t* item)
{
    actor_destroy(item->actor);
}



void checkpointorb_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    checkpointorb_t *me = (checkpointorb_t*)item;
    actor_t *act = item->actor;
    int i;

    if(!me->is_active) {
        /* activating the checkpoint orb... */
        for(i=0; i<team_size; i++) {
            player_t *player = team[i];
            if(!player_is_dying(player) && player_collision(player, act)) {
                me->is_active = TRUE; /* I'm active! */
                sound_play( soundfactory_get("checkpoint orb") );
                level_set_spawn_point(act->position);
                actor_change_animation(act, sprite_get_animation("SD_CHECKPOINT", 1));
                break;
            }
        }
    }
    else {
        if(actor_animation_finished(act))
            actor_change_animation(act, sprite_get_animation("SD_CHECKPOINT", 2));
    }
}


void checkpointorb_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}

