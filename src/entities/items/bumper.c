/*
 * Open Surge Engine
 * bumper.c - bumper
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

#include "bumper.h"
#include "../../core/util.h"
#include "../../core/audio.h"
#include "../../core/soundfactory.h"
#include "../player.h"
#include "../brick.h"
#include "../item.h"
#include "../enemy.h"
#include "../actor.h"

/* bumper class */
typedef struct bumper_t bumper_t;
struct bumper_t {
    item_t item; /* base class */
    int getting_hit;
};

static void bumper_init(item_t *item);
static void bumper_release(item_t* item);
static void bumper_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void bumper_render(item_t* item, v2d_t camera_position);

static void bump(item_t *bumper, player_t *player);


/* public methods */
item_t* bumper_create()
{
    item_t *item = mallocx(sizeof(bumper_t));

    item->init = bumper_init;
    item->release = bumper_release;
    item->update = bumper_update;
    item->render = bumper_render;

    return item;
}


/* private methods */
void bumper_init(item_t *item)
{
    bumper_t *me = (bumper_t*)item;

    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    me->getting_hit = FALSE;
    actor_change_animation(item->actor, sprite_get_animation("SD_BUMPER", 0));
}



void bumper_release(item_t* item)
{
    actor_destroy(item->actor);
}



void bumper_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    bumper_t *me = (bumper_t*)item;
    actor_t *act = item->actor;
    int i;

    for(i=0; i<team_size; i++) {
        player_t *player = team[i];
        if(!player_is_dying(player) && actor_pixelperfect_collision(player->actor, act)) {
            if(!me->getting_hit) {
                me->getting_hit = TRUE;
                actor_change_animation(act, sprite_get_animation("SD_BUMPER", 1));
                sound_play( soundfactory_get("bumper") );
                bump(item, player);
            }
        }
    }

    if(me->getting_hit) {
        if(actor_animation_finished(act)) {
            me->getting_hit = FALSE;
            actor_change_animation(act, sprite_get_animation("SD_BUMPER", 0));
        }
    }
}


void bumper_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}

/* misc */
void bump(item_t *bumper, player_t *player)
{
    /* law of conservation of linear momentum */
    float ec = 1.0f; /* (coefficient of restitution == 1.0) => elastic collision */
    float mass_player = 1.0f;
    float mass_bumper = 10000.0f;
    float mass_ratio = mass_bumper / mass_player;
    v2d_t v0, approximation_speed, separation_speed;
    actor_t *act = bumper->actor;



    v0 = player->actor->speed; /* initial speed of the player */
    v0.x = (v0.x < 0) ? min(-300, v0.x) : max(300, v0.x);



    approximation_speed = v2d_multiply(
        v2d_normalize(
            v2d_subtract(act->position, player->actor->position)
        ),
        v2d_magnitude(v0)
    );

    separation_speed = v2d_multiply(approximation_speed, ec);



    player->actor->speed = v2d_multiply(
        v2d_add(
            v0,
            v2d_multiply(separation_speed, -mass_ratio)
        ),
        1.0f / (mass_ratio + 1.0f)
    );

    act->speed = v2d_multiply(
        v2d_add(v0, separation_speed),
        1.0f / (mass_ratio + 1.0f)
    );
}

