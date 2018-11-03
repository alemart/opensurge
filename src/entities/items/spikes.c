/*
 * Open Surge Engine
 * spikes.c - spikes
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

#include "spikes.h"
#include "../../core/util.h"
#include "../../core/stringutil.h"
#include "../../core/audio.h"
#include "../../core/timer.h"
#include "../../core/soundfactory.h"
#include "../player.h"
#include "../brick.h"
#include "../item.h"
#include "../enemy.h"
#include "../actor.h"

/* spikes class */
typedef struct spikes_t spikes_t;
struct spikes_t {
    item_t item; /* base class */
    int (*collision)(item_t*,player_t*); /* strategy pattern */
    int anim_id; /* animation number */
    float timer; /* periodic spikes */
    float cycle_length; /* periodic spikes */
    int hidden; /* is this object hidden? (it shows and hides every cycle) */
};

static item_t* spikes_create(int (*collision)(item_t*,player_t*), int anim_id, float cycle_length);
static void spikes_init(item_t *item);
static void spikes_release(item_t* item);
static void spikes_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void spikes_render(item_t* item, v2d_t camera_position);

static int hittest(player_t *player, float rect[4]);

static int floor_strategy(item_t *spikes, player_t *player);
static int ceiling_strategy(item_t *spikes, player_t *player);
static int leftwall_strategy(item_t *spikes, player_t *player);
static int rightwall_strategy(item_t *spikes, player_t *player);



/* public methods */
item_t* floorspikes_create()
{
    return spikes_create(floor_strategy, 0, INFINITY_FLT);
}

item_t* ceilingspikes_create()
{
    return spikes_create(ceiling_strategy, 2, INFINITY_FLT);
}

item_t* leftwallspikes_create()
{
    return spikes_create(leftwall_strategy, 1, INFINITY_FLT);
}

item_t* rightwallspikes_create()
{
    return spikes_create(rightwall_strategy, 3, INFINITY_FLT);
}

item_t* periodic_floorspikes_create()
{
    return spikes_create(floor_strategy, 0, 5.0f);
}

item_t* periodic_ceilingspikes_create()
{
    return spikes_create(ceiling_strategy, 2, 5.0f);
}

item_t* periodic_leftwallspikes_create()
{
    return spikes_create(leftwall_strategy, 1, 5.0f);
}

item_t* periodic_rightwallspikes_create()
{
    return spikes_create(rightwall_strategy, 3, 5.0f);
}

/* private methods */
item_t* spikes_create(int (*collision)(item_t*,player_t*), int anim_id, float cycle_length)
{
    item_t *item = mallocx(sizeof(spikes_t));
    spikes_t *me = (spikes_t*)item;

    item->init = spikes_init;
    item->release = spikes_release;
    item->update = spikes_update;
    item->render = spikes_render;

    me->collision = collision;
    me->anim_id = anim_id;
    me->cycle_length = cycle_length;

    return item;
}

void spikes_init(item_t *item)
{
    spikes_t *me = (spikes_t*)item;

    item->always_active = FALSE;
    item->obstacle = TRUE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    me->timer = 0.0f;
    me->hidden = FALSE;

    actor_change_animation(item->actor, sprite_get_animation("SD_SPIKES", me->anim_id));
}



void spikes_release(item_t* item)
{
    actor_destroy(item->actor);
}



void spikes_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    spikes_t *me = (spikes_t*)item;
    float dt = timer_get_delta();

    /* change state */
    me->timer += dt;
    if(me->timer >= me->cycle_length * 0.5f) {
        me->timer = 0.0f;
        me->hidden = !me->hidden;
        sound_play(
            soundfactory_get(
                me->hidden ? "spikes disappearing" : "spikes appearing"
            )
        );
    }
    item->obstacle = !me->hidden;
    item->actor->visible = !me->hidden;

    /* spike collision */
    if(!me->hidden) {
        int i;

        for(i=0; i<team_size; i++) {
            player_t *player = team[i];
            if(!player_is_dying(player) && !player_is_getting_hit(player) && !player->blinking && !player_is_invincible(player)) {
                if(me->collision(item, player)) {
                    sound_t *s = soundfactory_get("spikes hit");
                    if(!sound_is_playing(s))
                        sound_play(s);
                    player_hit_ex(player, item->actor);
                }
            }
        }
    }
}


void spikes_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}

/* private strategies */
int floor_strategy(item_t *spikes, player_t *player)
{
    float b[4];
    float feet;
    actor_t *act = spikes->actor;

    b[0] = act->position.x - act->hot_spot.x + 5;
    b[1] = act->position.y - act->hot_spot.y - 5;
    b[2] = b[0] + image_width(actor_image(act)) - 10;
    b[3] = b[1] + 10;

    feet = player->actor->position.y - player->actor->hot_spot.y + image_height(actor_image(player->actor));
    return hittest(player, b) && feet < (act->position.y - act->hot_spot.y + image_height(actor_image(act))/2);
}

int ceiling_strategy(item_t *spikes, player_t *player)
{
    float b[4];
    actor_t *act = spikes->actor;

    b[0] = act->position.x - act->hot_spot.x + 5;
    b[1] = act->position.y - act->hot_spot.y + image_height(actor_image(act)) - 5;
    b[2] = b[0] + image_width(actor_image(act)) - 10;
    b[3] = b[1] + 10;

    return hittest(player, b);
}

int leftwall_strategy(item_t *spikes, player_t *player)
{
    float b[4];
    actor_t *act = spikes->actor;

    b[0] = act->position.x - act->hot_spot.x + image_width(actor_image(act)) - 5;
    b[1] = act->position.y - act->hot_spot.y + 5;
    b[2] = b[0] + 10;
    b[3] = b[1] + image_height(actor_image(act)) - 10;

    return hittest(player, b);
}

int rightwall_strategy(item_t *spikes, player_t *player)
{
    float b[4];
    actor_t *act = spikes->actor;

    b[0] = act->position.x - act->hot_spot.x - 5;
    b[1] = act->position.y - act->hot_spot.y + 5;
    b[2] = b[0] + 10;
    b[3] = b[1] + image_height(actor_image(act)) - 10;

    return hittest(player, b);
}


/* misc */

/* returns true if the player collides with the given rectangle */
int hittest(player_t *player, float rect[4])
{
    float a[4];
    actor_t *pl = player->actor;

    a[0] = pl->position.x - pl->hot_spot.x;
    a[1] = pl->position.y - pl->hot_spot.y;
    a[2] = a[0] + image_width(actor_image(pl));
    a[3] = a[1] + image_height(actor_image(pl));

    return bounding_box(a, rect);
}

