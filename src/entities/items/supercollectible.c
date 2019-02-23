/*
 * Open Surge Engine
 * supercollectible.c - super collectible
 * Copyright (C) 2010, 2014  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include "supercollectible.h"
#include "../../core/util.h"
#include "../../core/soundfactory.h"
#include "../../scenes/level.h"
#include "../player.h"
#include "../brick.h"
#include "../item.h"
#include "../enemy.h"
#include "../actor.h"

/* supercollectible class */
typedef struct supercollectible_t supercollectible_t;
struct supercollectible_t {
    item_t item; /* base class */
    int is_disappearing; /* is the disappearing animation being played? */
};

static void supercollectible_init(item_t *item);
static void supercollectible_release(item_t* item);
static void supercollectible_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void supercollectible_render(item_t* item, v2d_t camera_position);



/* public methods */
item_t* supercollectible_create()
{
    item_t *item = mallocx(sizeof(supercollectible_t));

    item->init = supercollectible_init;
    item->release = supercollectible_release;
    item->update = supercollectible_update;
    item->render = supercollectible_render;

    return item;
}


/* private methods */
void supercollectible_init(item_t *item)
{
    supercollectible_t *me = (supercollectible_t*)item;

    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    me->is_disappearing = FALSE;
    actor_change_animation(item->actor, sprite_get_animation("SD_SUPERCOLLECTIBLE", 0));
    actor_synchronize_animation(item->actor, TRUE);
}



void supercollectible_release(item_t* item)
{
    actor_destroy(item->actor);
}



void supercollectible_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    player_t *player = level_player();
    supercollectible_t *me = (supercollectible_t*)item;
    actor_t *act = item->actor;

    act->visible = (player->got_glasses || level_editmode());

    if(!me->is_disappearing) {
        if(!player_is_dying(player) && player->got_glasses && player_collision(player, act)) {
            /* the player is capturing this ring */
            actor_change_animation(act, sprite_get_animation("SD_SUPERCOLLECTIBLE", 1));
            player_set_collectibles( player_get_collectibles() + 5 );
            sound_play( soundfactory_get("super collectible") );
            me->is_disappearing = TRUE;
        }
    }
    else {
        if(actor_animation_finished(act)) {
            /* ouch, I've been caught! It's time to disappear... */
            item->state = IS_DEAD;
        }
    }
}


void supercollectible_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}

