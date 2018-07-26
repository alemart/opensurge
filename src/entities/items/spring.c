/*
 * Open Surge Engine
 * spring.c - spring
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

#include <math.h>
#include "spring.h"
#include "../../core/util.h"
#include "../../core/audio.h"
#include "../../core/timer.h"
#include "../../core/stringutil.h"
#include "../../core/soundfactory.h"
#include "../player.h"
#include "../brick.h"
#include "../item.h"
#include "../enemy.h"
#include "../actor.h"

/* constants */
#define SPRING_BANG_TIMER           0.2 /* sfx control */

/* spring class */
typedef struct spring_t spring_t;
struct spring_t {
    item_t item; /* base class */
    v2d_t strength;
    char *sprite_name;
    float bang_timer;
    int is_bumping;
    void (*on_bump)(item_t*,player_t*); /* strategy pattern */
};

static item_t* spring_create(void (*strategy)(item_t*,player_t*), const char *sprite_name, v2d_t strength);
static void spring_init(item_t *item);
static void spring_release(item_t* item);
static void spring_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void spring_render(item_t* item, v2d_t camera_position);

static void classicspring_strategy(item_t *item, player_t *player); /* activates if you jump on it */
static void volatilespring_strategy(item_t *item, player_t *player); /* activates when touched */

static void springfy_player(player_t *player, v2d_t strength);
static void activate_spring(spring_t *spring, player_t *player);



/* public methods */
item_t* yellowspring_create() /* regular spring */
{
    return spring_create(classicspring_strategy, "SD_YELLOWSPRING", v2d_new(0,-600));
}

item_t* tryellowspring_create() /* top-right */
{
    return spring_create(volatilespring_strategy, "SD_TRYELLOWSPRING", v2d_new(424,-424));
}

item_t* ryellowspring_create()  /* right-oriented spring */
{
    return spring_create(volatilespring_strategy, "SD_RYELLOWSPRING", v2d_new(600,0));
}

item_t* bryellowspring_create() /* bottom-right */
{
    return spring_create(volatilespring_strategy, "SD_BRYELLOWSPRING", v2d_new(424,424));
}

item_t* byellowspring_create() /* bottom-oriented spring */
{
    return spring_create(volatilespring_strategy, "SD_BYELLOWSPRING", v2d_new(0,600));
}

item_t* blyellowspring_create() /* bottom-left */
{
    return spring_create(volatilespring_strategy, "SD_BLYELLOWSPRING", v2d_new(-424,424));
}

item_t* lyellowspring_create() /* left-oriented spring */
{
    return spring_create(volatilespring_strategy, "SD_LYELLOWSPRING", v2d_new(-600,0));
}

item_t* tlyellowspring_create() /* top-left */
{
    return spring_create(volatilespring_strategy, "SD_TLYELLOWSPRING", v2d_new(-424,-424));
}

item_t* redspring_create() /* regular spring */
{
    return spring_create(classicspring_strategy, "SD_REDSPRING", v2d_new(0,-960));
}

item_t* trredspring_create() /* top-right */
{
    return spring_create(volatilespring_strategy, "SD_TRREDSPRING", v2d_new(679,-679));
}

item_t* rredspring_create()  /* right-oriented spring */
{
    return spring_create(volatilespring_strategy, "SD_RREDSPRING", v2d_new(960,0));
}

item_t* brredspring_create() /* bottom-right */
{
    return spring_create(volatilespring_strategy, "SD_BRREDSPRING", v2d_new(679,679));
}

item_t* bredspring_create() /* bottom-oriented spring */
{
    return spring_create(volatilespring_strategy, "SD_BREDSPRING", v2d_new(0,960));
}

item_t* blredspring_create() /* bottom-left */
{
    return spring_create(volatilespring_strategy, "SD_BLREDSPRING", v2d_new(-679,679));
}

item_t* lredspring_create() /* left-oriented spring */
{
    return spring_create(volatilespring_strategy, "SD_LREDSPRING", v2d_new(-960,0));
}

item_t* tlredspring_create() /* top-left */
{
    return spring_create(volatilespring_strategy, "SD_TLREDSPRING", v2d_new(-679,-679));
}

item_t* bluespring_create() /* regular spring */
{
    return spring_create(classicspring_strategy, "SD_BLUESPRING", v2d_new(0,-1500));
}

item_t* trbluespring_create() /* top-right */
{
    return spring_create(volatilespring_strategy, "SD_TRBLUESPRING", v2d_new(1061,-1061));
}

item_t* rbluespring_create()  /* right-oriented spring */
{
    return spring_create(volatilespring_strategy, "SD_RBLUESPRING", v2d_new(1500,0));
}

item_t* brbluespring_create() /* bottom-right */
{
    return spring_create(volatilespring_strategy, "SD_BRBLUESPRING", v2d_new(1061,1061));
}

item_t* bbluespring_create() /* bottom-oriented spring */
{
    return spring_create(volatilespring_strategy, "SD_BBLUESPRING", v2d_new(0,1500));
}

item_t* blbluespring_create() /* bottom-left */
{
    return spring_create(volatilespring_strategy, "SD_BLBLUESPRING", v2d_new(-1061,1061));
}

item_t* lbluespring_create() /* left-oriented spring */
{
    return spring_create(volatilespring_strategy, "SD_LBLUESPRING", v2d_new(-1500,0));
}

item_t* tlbluespring_create() /* top-left */
{
    return spring_create(volatilespring_strategy, "SD_TLBLUESPRING", v2d_new(-1061,-1061));
}


/* private strategies */

/* springs using the volatile strategy are activated when you touch them */
void volatilespring_strategy(item_t *item, player_t *player)
{
    activate_spring((spring_t*)item, player);
}

/* springs using the classic strategy are activated when you jump on them */
void classicspring_strategy(item_t *item, player_t *player)
{
    if(player->actor->speed.y >= 1.0f || fabs(player->actor->angle) > EPSILON)
        activate_spring((spring_t*)item, player);
}

/* private methods */
item_t* spring_create(void (*strategy)(item_t*,player_t*), const char *sprite_name, v2d_t strength)
{
    item_t *item = mallocx(sizeof(spring_t));
    spring_t *me = (spring_t*)item;

    item->init = spring_init;
    item->release = spring_release;
    item->update = spring_update;
    item->render = spring_render;

    me->on_bump = strategy;
    me->sprite_name = str_dup(sprite_name);
    me->strength = strength;

    return item;
}


/* private methods */
void spring_init(item_t *item)
{
    spring_t *me = (spring_t*)item;

    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    me->is_bumping = FALSE;
    me->bang_timer = 0.0f;
    actor_change_animation(item->actor, sprite_get_animation(me->sprite_name, 0));
}



void spring_release(item_t* item)
{
    spring_t *me = (spring_t*)item;

    actor_destroy(item->actor);
    free(me->sprite_name);
}



void spring_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    spring_t *me = (spring_t*)item;
    float dt = timer_get_delta();
    int i;

    /* bump! */
    me->bang_timer += dt;
    for(i=0; i<team_size; i++) {
        player_t *player = team[i];
        if(!player_is_dying(player) && actor_pixelperfect_collision(player->actor, item->actor))
            me->on_bump(item, player);
    }

    /* restore default animation */
    if(me->is_bumping) {
        if(actor_animation_finished(item->actor)) {
            actor_change_animation(item->actor, sprite_get_animation(me->sprite_name, 0));
            me->is_bumping = FALSE;
        }
    }
}


void spring_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}


/* 'springfy' player */
void springfy_player(player_t *player, v2d_t strength)
{
    actor_t *act = player->actor;

    if(fabs(strength.y) > EPSILON && fabs(strength.x) > EPSILON)
        act->speed = strength;
    else if(fabs(strength.y) > EPSILON)
        act->speed.y = strength.y;
    else if(fabs(strength.x) > EPSILON) {
        act->speed.x = strength.x;
        player_lock_horizontally_for(player, 0.27f);
    }
}

/* activate the spring */
void activate_spring(spring_t *spring, player_t *player)
{
    item_t *item = (item_t*)spring;

    spring->is_bumping = TRUE;
    springfy_player(player, spring->strength);
    actor_change_animation(item->actor, sprite_get_animation(spring->sprite_name, 1));

    if(spring->strength.y < 0.0f) {
        player->actor->position.y -= 2; /* bugfix */
        player_spring(player);
    }

    if(spring->strength.x > EPSILON)
        player->actor->mirror &= ~IF_HFLIP;
    else if(spring->strength.x < -EPSILON)
        player->actor->mirror |= IF_HFLIP;
    else
        player_spring(player);

    if(spring->bang_timer > SPRING_BANG_TIMER) {
        sound_play( soundfactory_get("spring") );
        spring->bang_timer = 0.0f;
    }
}

