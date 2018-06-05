/*
 * Open Surge Engine
 * bigring.c - big ring
 * Copyright (C) 2010  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "bigring.h"
#include "../../core/util.h"
#include "../../core/soundfactory.h"
#include "../../scenes/level.h"
#include "../player.h"
#include "../brick.h"
#include "../item.h"
#include "../enemy.h"
#include "../actor.h"

/* bigring class */
typedef struct bigring_t bigring_t;
struct bigring_t {
    item_t item; /* base class */
};

static void bigring_init(item_t *item);
static void bigring_release(item_t* item);
static void bigring_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void bigring_render(item_t* item, v2d_t camera_position);



/* public methods */
item_t* bigring_create()
{
    item_t *item = mallocx(sizeof(bigring_t));

    item->init = bigring_init;
    item->release = bigring_release;
    item->update = bigring_update;
    item->render = bigring_render;

    return item;
}


/* private methods */
void bigring_init(item_t *item)
{
    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = FALSE;
    item->preserve = TRUE;
    item->actor = actor_create();

    actor_change_animation(item->actor, sprite_get_animation("SD_BIGRING", 0));
    actor_synchronize_animation(item->actor, TRUE);
}



void bigring_release(item_t* item)
{
    actor_destroy(item->actor);
}



void bigring_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    int i;

    for(i=0; i<team_size; i++) {
        player_t *player = team[i];
        if(!player_is_dying(player) && actor_pixelperfect_collision(player->actor, item->actor)) {
            item->state = IS_DEAD;
            player_set_collectibles( player_get_collectibles() + 50 );
            sound_play( soundfactory_get("big ring") );
        }
    }
}


void bigring_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}

