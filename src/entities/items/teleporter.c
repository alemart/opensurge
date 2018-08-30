/*
 * Open Surge Engine
 * teleporter.c - teleporter
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

#include "teleporter.h"
#include "../../core/util.h"
#include "../../core/input.h"
#include "../../core/audio.h"
#include "../../core/timer.h"
#include "../../core/soundfactory.h"
#include "../../scenes/level.h"
#include "../player.h"
#include "../brick.h"
#include "../item.h"
#include "../enemy.h"
#include "../actor.h"

/* teleporter class */
typedef struct teleporter_t teleporter_t;
struct teleporter_t {
    item_t item; /* base class */
    int is_disabled; /* is this teleporter disabled? */
    int is_active; /* is this object teleporting the team? */
    float timer; /* time counter */
    player_t *who; /* who has activated me? */
};

static void teleporter_init(item_t *item);
static void teleporter_release(item_t* item);
static void teleporter_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void teleporter_render(item_t* item, v2d_t camera_position);

static void teleport_player_to(player_t *player, v2d_t position);



/* public methods */
item_t* teleporter_create()
{
    item_t *item = mallocx(sizeof(teleporter_t));

    item->init = teleporter_init;
    item->release = teleporter_release;
    item->update = teleporter_update;
    item->render = teleporter_render;

    return item;
}

void teleporter_activate(item_t *teleporter, player_t *who)
{
    teleporter_t *me = (teleporter_t*)teleporter;
    actor_t *act = teleporter->actor;

    if(!me->is_active && !me->is_disabled) {
        me->is_active = TRUE;
        me->who = who;

        input_ignore(who->actor->input);
        level_set_camera_focus(act);
        sound_play( soundfactory_get("teleporter") );
    }
}

/* private methods */
void teleporter_init(item_t *item)
{
    teleporter_t *me = (teleporter_t*)item;

    item->always_active = TRUE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    me->is_disabled = FALSE;
    me->is_active = FALSE;
    me->timer = 0.0f;

    actor_change_animation(item->actor, sprite_get_animation("SD_TELEPORTER", 0));
}



void teleporter_release(item_t* item)
{
    actor_destroy(item->actor);
}



void teleporter_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    teleporter_t *me = (teleporter_t*)item;
    actor_t *act = item->actor;
    float dt = timer_get_delta();
    int i, k=0;
    
    if(me->is_active) {
        me->timer += dt;
        if(me->timer >= 3.0f) {
            /* okay, teleport them all! */
            player_t *who = me->who; /* who has activated the teleporter? */

            input_restore(who->actor->input);
            level_set_camera_focus(who->actor);

            for(i=0; i<team_size; i++) {
                player_t *player = team[i];
                if(player != who) {
                    v2d_t position = v2d_add(act->position, v2d_new(-20 + 40*(k++), -30));
                    teleport_player_to(player, position);
                }
            }

            me->is_active = FALSE;
            me->is_disabled = TRUE; /* the teleporter works only once */
        }
        else {
            ; /* the players are being teletransported... wait a little bit. */
        }

        actor_change_animation(act, sprite_get_animation("SD_TELEPORTER", 1));
    }
    else
        actor_change_animation(act, sprite_get_animation("SD_TELEPORTER", 0));
}


void teleporter_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}

/* teleports the player to the given position */
void teleport_player_to(player_t *player, v2d_t position)
{
    player->actor->position = position;
    player->actor->speed = v2d_new(0,0);
    player->actor->angle = 0;
    player->bring_to_back = FALSE;
}

