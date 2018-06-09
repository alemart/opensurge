/*
 * Open Surge Engine
 * crushedbox.c - crushed box
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

#include "crushedbox.h"
#include "../../core/util.h"
#include "../player.h"
#include "../brick.h"
#include "../item.h"
#include "../enemy.h"
#include "../actor.h"

/* crushedbox class */
typedef struct crushedbox_t crushedbox_t;
struct crushedbox_t {
    item_t item; /* base class */
};

static void crushedbox_init(item_t *item);
static void crushedbox_release(item_t* item);
static void crushedbox_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void crushedbox_render(item_t* item, v2d_t camera_position);



/* public methods */
item_t* crushedbox_create()
{
    item_t *item = mallocx(sizeof(crushedbox_t));

    item->init = crushedbox_init;
    item->release = crushedbox_release;
    item->update = crushedbox_update;
    item->render = crushedbox_render;

    return item;
}


/* private methods */
void crushedbox_init(item_t *item)
{
    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    actor_change_animation(item->actor, sprite_get_animation("SD_ITEMBOX", 10));
}



void crushedbox_release(item_t* item)
{
    actor_destroy(item->actor);
}



void crushedbox_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    ; /* empty */
}


void crushedbox_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}

