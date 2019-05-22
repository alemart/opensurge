/*
 * Open Surge Engine
 * animalprison.c - animal prison (this object appears after the boss fight)
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

#include "animalprison.h"
#include "../../core/util.h"
#include "../../core/audio.h"
#include "../../core/timer.h"
#include "../../core/soundfactory.h"
#include "../../scenes/level.h"
#include "../player.h"
#include "../brick.h"
#include "../item.h"
#include "../enemy.h"
#include "../actor.h"



/*
   The animal prison is that object you hit at the end of the level
   in order to free the little animals. This is state machine:

   IDLE ---> EXPLODING ---> RELEASING THE ANIMALS ---> BROKEN

   We implement the State design pattern.
*/



/* Basic state (abstract) */
typedef struct state_t state_t; /* abstract class */
struct state_t {
    void (*handle)(state_t*,item_t*,player_t**,int); /* receives: state, animalprison, team, team_size */
};

/* Idle state: waiting to be hit... */
typedef struct state_idle_t state_idle_t; /* concrete state: idle */
struct state_idle_t {
    state_t state; /* base class */
    int being_hit; /* am I being hit? */
    int hit_count; /* the player has hit me 'hit_count' times */
};

static state_t* state_idle_new(); /* state constructor */
static void state_idle_handle(state_t*,item_t*,player_t**,int); /* private method */

/* After the animal prison got hit a few times, it explodes for a little while */
typedef struct state_exploding_t state_exploding_t; /* concrete state: exploding */
struct state_exploding_t {
    state_t state; /* base class */
    float explode_timer; /* explosion time accumulator */
    float break_timer; /* how long until it breaks... */
};

static state_t* state_exploding_new(); /* state constructor */
static void state_exploding_handle(state_t*,item_t*,player_t**,int); /* private method */

/* After it explodes, it must release the little animals ;) */
typedef struct state_releasing_t state_releasing_t; /* concrete state: releasing */
struct state_releasing_t {
    state_t state;
};

static state_t* state_releasing_new(); /* state constructor */
static void state_releasing_handle(state_t*,item_t*,player_t**,int); /* private method */

/* This is finally broken */
typedef struct state_broken_t state_broken_t; /* concrete state: broken */
struct state_broken_t {
    state_t state;
};

static state_t* state_broken_new(); /* state constructor */
static void state_broken_handle(state_t*,item_t*,player_t**,int); /* private method */



/* animalprison class */
typedef struct animalprison_t animalprison_t;
struct animalprison_t {
    item_t item; /* base class */
    state_t *state; /* state pattern */
};

static void animalprison_init(item_t *item);
static void animalprison_release(item_t* item);
static void animalprison_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void animalprison_render(item_t* item, v2d_t camera_position);

static void animalprison_set_state(item_t *item, state_t *state);
static int animalprison_got_hit_by_player(item_t *item, player_t *player);



/* public methods */
item_t* animalprison_create()
{
    item_t *item = mallocx(sizeof(animalprison_t));
    animalprison_t *me = (animalprison_t*)item;

    item->init = animalprison_init;
    item->release = animalprison_release;
    item->update = animalprison_update;
    item->render = animalprison_render;

    me->state = NULL;

    return item;
}



/* private methods */
void animalprison_set_state(item_t *item, state_t *state)
{
    animalprison_t *me = (animalprison_t*)item;

    if(me->state != NULL)
        free(me->state);

    me->state = state;
}

void animalprison_init(item_t *item)
{
    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    animalprison_set_state(item, state_idle_new());
    actor_change_animation(item->actor, sprite_get_animation("SD_ENDLEVEL", 0));
}

void animalprison_release(item_t* item)
{
    actor_destroy(item->actor);
    animalprison_set_state(item, NULL);
}

void animalprison_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    animalprison_t *me = (animalprison_t*)item;
    me->state->handle(me->state, item, team, team_size);
}

void animalprison_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}

/* states: constructors */
state_t* state_idle_new()
{
    state_t *base = mallocx(sizeof(state_idle_t));
    state_idle_t *derived = (state_idle_t*)base;

    base->handle = state_idle_handle;
    derived->being_hit = FALSE;
    derived->hit_count = 0;

    return base;
}

state_t* state_exploding_new()
{
    state_t *base = mallocx(sizeof(state_exploding_t));
    state_exploding_t *derived = (state_exploding_t*)base;

    base->handle = state_exploding_handle;
    derived->explode_timer = 0.0f;
    derived->break_timer = 0.0f;

    return base;
}

state_t* state_releasing_new()
{
    state_t *base = mallocx(sizeof(state_releasing_t));
    base->handle = state_releasing_handle;
    return base;
}

state_t* state_broken_new()
{
    state_t *base = mallocx(sizeof(state_broken_t));
    base->handle = state_broken_handle;
    return base;
}

/* implementation of the states */
void state_idle_handle(state_t *state, item_t *item, player_t **team, int team_size)
{
    int i;
    state_idle_t *s = (state_idle_t*)state;
    actor_t *act = item->actor;

    for(i=0; i<team_size; i++) {
        player_t *player = team[i];
        if(animalprison_got_hit_by_player(item, player) && !s->being_hit) {
            /* oh no! the player is attacking this object! */
            s->being_hit = TRUE;
            actor_change_animation(act, sprite_get_animation("SD_ENDLEVEL", 1));
            sound_play( soundfactory_get("boss hit") );
            player_bounce_ex(player, act, FALSE);
            player->actor->speed.x *= -0.5;

            if(++(s->hit_count) >= 3) /* 3 hits and you're done */
                animalprison_set_state(item, state_exploding_new());
        }
    }

    /* after getting hit, restore the animation */
    if(actor_animation_finished(act) && s->being_hit) {
        actor_change_animation(act, sprite_get_animation("SD_ENDLEVEL", 0));
        s->being_hit = FALSE;
    }
}

void state_exploding_handle(state_t *state, item_t *item, player_t **team, int team_size)
{
    state_exploding_t *s = (state_exploding_t*)state;
    actor_t *act = item->actor;
    float dt = timer_get_delta();

    s->explode_timer += dt;
    s->break_timer += dt;

    /* keep exploding for a while... */
    if(s->explode_timer >= 0.1f) {
        v2d_t pos = v2d_new(
            act->position.x - act->hot_spot.x + random(image_width(actor_image(act))),
            act->position.y - act->hot_spot.y + random(image_height(actor_image(act))/2)
        );
        level_create_item(IT_EXPLOSION, pos);
        sound_play( soundfactory_get("explode") );

        s->explode_timer = 0.0f;
    }

    /* okay, I can't explode anymore */
    if(s->break_timer >= 2.0f)
        animalprison_set_state(item, state_releasing_new());
}

void state_releasing_handle(state_t *state, item_t *item, player_t **team, int team_size)
{
    actor_t *act = item->actor;
    int i;

    /* release the animals! */
    for(i=0; i<20; i++) {
        v2d_t pos = v2d_new(
            act->position.x - act->hot_spot.x + random(image_width(actor_image(act))),
            act->position.y - act->hot_spot.y + random(image_height(actor_image(act))/2)
        );
        level_create_animal(pos);
    }

    /* congratulations! you have just cleared the level! */
    level_clear(act);

    /* sayonara bye bye */
    actor_change_animation(act, sprite_get_animation("SD_ENDLEVEL", 2));
    animalprison_set_state(item, state_broken_new());
}

void state_broken_handle(state_t *state, item_t *item, player_t **team, int team_size)
{
    ; /* do nothing */
}

/* misc */
/* returns true if the animal prison got hit by the given player */
int animalprison_got_hit_by_player(item_t *item, player_t *player)
{
    float a[4], b[4];
    actor_t *act = item->actor;
    actor_t *pl = player->actor;

    a[0] = pl->position.x - pl->hot_spot.x;
    a[1] = pl->position.y - pl->hot_spot.y;
    a[2] = a[0] + image_width(actor_image(pl));
    a[3] = a[1] + image_height(actor_image(pl));

    b[0] = act->position.x - act->hot_spot.x + 5;
    b[1] = act->position.y - act->hot_spot.y;
    b[2] = b[0] + image_width(actor_image(act)) - 10;
    b[3] = b[1] + image_height(actor_image(act))/2;

    return (player_is_attacking(player) && bounding_box(a,b) && player_collision(player, act));
}

