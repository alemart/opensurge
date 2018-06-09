/*
 * Open Surge Engine
 * endsign.c - end sign (touch it to clear the level)
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

#include "endsign.h"
#include "../../core/util.h"
#include "../../core/stringutil.h"
#include "../../core/soundfactory.h"
#include "../../scenes/level.h"
#include "../player.h"
#include "../brick.h"
#include "../item.h"
#include "../enemy.h"
#include "../actor.h"

/* endsign class */
typedef struct endsign_t endsign_t;
struct endsign_t {
    item_t item; /* base class */
    player_t *who; /* who has touched the end sign? */
};

static void endsign_init(item_t *item);
static void endsign_release(item_t* item);
static void endsign_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void endsign_render(item_t* item, v2d_t camera_position);



/* public methods */
item_t* endsign_create()
{
    item_t *item = mallocx(sizeof(endsign_t));

    item->init = endsign_init;
    item->release = endsign_release;
    item->update = endsign_update;
    item->render = endsign_render;

    return item;
}


/* private methods */
void endsign_init(item_t *item)
{
    endsign_t *me = (endsign_t*)item;

    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    me->who = NULL;
    actor_change_animation(item->actor, sprite_get_animation("SD_ENDSIGN", 0));
}



void endsign_release(item_t* item)
{
    actor_destroy(item->actor);
}



void endsign_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    endsign_t *me = (endsign_t*)item;
    actor_t *act = item->actor;

    if(me->who == NULL) {
        /* I haven't been touched yet */
        int i;

        for(i=0; i<team_size; i++) {
            player_t *player = team[i];
            if(!player_is_dying(player) && actor_pixelperfect_collision(player->actor, act)) {
                me->who = player; /* I have just been touched by 'player' */
                sound_play( soundfactory_get("end sign") );
                actor_change_animation(act, sprite_get_animation("SD_ENDSIGN", 1));
                level_clear(item->actor);
            }
        }
    }
    else {
        /* me->who has touched me! */
        if(actor_animation_finished(act)) {
            int anim_id;

            if(str_icmp(me->who->name, "Surge") == 0)
                anim_id = 2;
            else if(str_icmp(me->who->name, "Neon") == 0)
                anim_id = 3;
            else if(str_icmp(me->who->name, "Charge") == 0)
                anim_id = 4;
            else
                anim_id = 5;

            actor_change_animation(act, sprite_get_animation("SD_ENDSIGN", anim_id));
        }
    }
}


void endsign_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}

