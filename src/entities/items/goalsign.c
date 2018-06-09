/*
 * Open Surge Engine
 * goalsign.c - goal sign
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

#include "goalsign.h"
#include "util/itemutil.h"
#include "../../core/util.h"
#include "../player.h"
#include "../brick.h"
#include "../item.h"
#include "../enemy.h"
#include "../actor.h"

/* goalsign class */
typedef struct goalsign_t goalsign_t;
struct goalsign_t {
    item_t item; /* base class */
};

static void goalsign_init(item_t *item);
static void goalsign_release(item_t* item);
static void goalsign_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void goalsign_render(item_t* item, v2d_t camera_position);

/* public methods */
item_t* goalsign_create()
{
    item_t *item = mallocx(sizeof(goalsign_t));

    item->init = goalsign_init;
    item->release = goalsign_release;
    item->update = goalsign_update;
    item->render = goalsign_render;

    return item;
}


/* private methods */
void goalsign_init(item_t *item)
{
    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    actor_change_animation(item->actor, sprite_get_animation("SD_GOAL", 0));
}



void goalsign_release(item_t* item)
{
    actor_destroy(item->actor);
}



void goalsign_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    item_t *endsign;
    int anim;

    endsign = find_closest_item(item, item_list, IT_ENDSIGN, NULL);
    if(endsign != NULL) {
        if(endsign->actor->position.x > item->actor->position.x)
            anim = 0;
        else
            anim = 1;
    }
    else
        anim = 0;

    actor_change_animation(item->actor, sprite_get_animation("SD_GOAL", anim));
}


void goalsign_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}

