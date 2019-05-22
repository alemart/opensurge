/*
 * Open Surge Engine
 * danger.c - danger item (useful on spikes)
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

#include "danger.h"
#include "../../core/util.h"
#include "../../core/stringutil.h"
#include "../../scenes/level.h"
#include "../player.h"
#include "../brick.h"
#include "../item.h"
#include "../enemy.h"
#include "../actor.h"

/* danger class */
typedef struct danger_t danger_t;
struct danger_t {
    item_t item; /* base class */
    char *sprite_name;
    int (*player_is_vulnerable)(player_t*);
};

static item_t* danger_create(const char *sprite_name, int (*player_is_vulnerable)(player_t*));
static void danger_init(item_t *item);
static void danger_release(item_t* item);
static void danger_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void danger_render(item_t* item, v2d_t camera_position);

static int always_vulnerable(player_t *player);
static int can_defend_against_fire(player_t *player);


/* public methods */
item_t* horizontaldanger_create()
{
    return danger_create("SD_DANGER", always_vulnerable);
}

item_t* verticaldanger_create()
{
    return danger_create("SD_VERTICALDANGER", always_vulnerable);
}

item_t* horizontalfiredanger_create()
{
    return danger_create("SD_FIREDANGER", can_defend_against_fire);
}

item_t* verticalfiredanger_create()
{
    return danger_create("SD_VERTICALFIREDANGER", can_defend_against_fire);
}


/* private methods */
item_t* danger_create(const char *sprite_name, int (*player_is_vulnerable)(player_t*))
{
    item_t *item = mallocx(sizeof(danger_t));
    danger_t *me = (danger_t*)item;

    item->init = danger_init;
    item->release = danger_release;
    item->update = danger_update;
    item->render = danger_render;

    me->sprite_name = str_dup(sprite_name);
    me->player_is_vulnerable = player_is_vulnerable;

    return item;
}

void danger_init(item_t *item)
{
    danger_t *me = (danger_t*)item;

    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    actor_change_animation(item->actor, sprite_get_animation(me->sprite_name, 0));
}



void danger_release(item_t* item)
{
    danger_t *me = (danger_t*)item;

    actor_destroy(item->actor);
    free(me->sprite_name);
}



void danger_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    int i;
    danger_t *me = (danger_t*)item;
    actor_t *act = item->actor;

    for(i=0; i<team_size; i++) {
        player_t *player = team[i];
        if(!player_is_dying(player) && !player->blinking && !player_is_invincible(player) && player_collision(player, act)) {
            if(me->player_is_vulnerable(player))
                player_hit_ex(player, act);
        }
    }

    act->visible = level_editmode();
}


void danger_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}

/* misc */
int always_vulnerable(player_t *player)
{
    return TRUE;
}

int can_defend_against_fire(player_t *player)
{
    return (player_shield_type(player) != SH_FIRESHIELD);
}
