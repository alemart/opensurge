/*
 * Open Surge Engine
 * loop.c - loop system
 * Copyright (C) 2011  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include "loop.h"
#include "../../core/util.h"
#include "../player.h"
#include "../brick.h"
#include "../item.h"
#include "../enemy.h"
#include "../actor.h"
#include "../../scenes/level.h"

/* loop class */
typedef struct loop_t loop_t;
struct loop_t {
    item_t item; /* base class */
    animation_t *animation;
    bricklayer_t layer_to_be_activated;

    int *player_was_touching_me;
    int team_size;
};

static item_t* loop_create(const char *sprite_name, bricklayer_t layer_to_be_activated);
static void loop_init(item_t *item);
static void loop_release(item_t* item);
static void loop_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void loop_render(item_t* item, v2d_t camera_position);



/* public methods */
item_t* loopgreen_create()
{
    return loop_create("SD_LOOPGREEN", BRL_GREEN);
}

item_t* loopyellow_create()
{
    return loop_create("SD_LOOPYELLOW", BRL_YELLOW);
}


/* private methods */
item_t* loop_create(const char *sprite_name, bricklayer_t layer_to_be_activated)
{
    loop_t *me = mallocx(sizeof *me);
    item_t *item = (item_t*)me;

    item->init = loop_init;
    item->release = loop_release;
    item->update = loop_update;
    item->render = loop_render;

    me->animation = sprite_get_animation(sprite_name, 0);
    me->layer_to_be_activated = layer_to_be_activated;
    me->player_was_touching_me = NULL;
    me->team_size = 0;

    return item;
}


void loop_init(item_t *item)
{
    loop_t *me = (loop_t*)item;

    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    actor_change_animation(item->actor, me->animation);
}



void loop_release(item_t* item)
{
    loop_t *me = (loop_t*)item;

    actor_destroy(item->actor);

    if(me->player_was_touching_me != NULL)
        free(me->player_was_touching_me);
}



void loop_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    int i;
    loop_t *me = (loop_t*)item;
    actor_t *act = item->actor;

    act->visible = level_editmode();

    if(team_size != me->team_size) {
        me->team_size = team_size;
        me->player_was_touching_me = realloc(me->player_was_touching_me, team_size * sizeof(int));
        for(i=0; i<team_size; i++)
            me->player_was_touching_me[i] = actor_pixelperfect_collision(team[i]->actor, act);
    }

    for(i=0; i<team_size; i++) {
        player_t *player = team[i];
        if(actor_pixelperfect_collision(player->actor, act)) {
            if(!me->player_was_touching_me[i]) {
                player->layer = me->layer_to_be_activated;
                me->player_was_touching_me[i] = TRUE;
            }
        }
        else
            me->player_was_touching_me[i] = FALSE;
    }
}


void loop_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}

