/*
 * Open Surge Engine
 * itembox.c - item boxes
 * Copyright (C) 2010, 2011  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include "itembox.h"
#include "icon.h"
#include "../../scenes/level.h"
#include "../../core/audio.h"
#include "../../core/util.h"
#include "../../core/stringutil.h"
#include "../../core/soundfactory.h"
#include "../player.h"
#include "../brick.h"
#include "../item.h"
#include "../enemy.h"
#include "../actor.h"

/* itembox class */
typedef struct itembox_t itembox_t;
struct itembox_t {
    item_t item; /* base class */
    int anim_id; /* animation id */
    void (*on_destroy)(item_t*,player_t*); /* strategy pattern */
};

static item_t* itembox_create(void (*on_destroy)(item_t*,player_t*), int anim_id);
static void itembox_init(item_t *item);
static void itembox_release(item_t* item);
static void itembox_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void itembox_render(item_t* item, v2d_t camera_position);

static void lifebox_strategy(item_t *item, player_t *player);
static void collectiblebox_strategy(item_t *item, player_t *player);
static void starbox_strategy(item_t *item, player_t *player);
static void speedbox_strategy(item_t *item, player_t *player);
static void glassesbox_strategy(item_t *item, player_t *player);
static void shieldbox_strategy(item_t *item, player_t *player);
static void fireshieldbox_strategy(item_t *item, player_t *player);
static void thundershieldbox_strategy(item_t *item, player_t *player);
static void watershieldbox_strategy(item_t *item, player_t *player);
static void acidshieldbox_strategy(item_t *item, player_t *player);
static void windshieldbox_strategy(item_t *item, player_t *player);
static void trapbox_strategy(item_t *item, player_t *player);
static void emptybox_strategy(item_t *item, player_t *player);

static int get_anim_id(const char *player_name);


/* public methods */
item_t* lifebox_create()
{
    return itembox_create(lifebox_strategy, 0); /* (strategy, animation id) */
}

item_t* collectiblebox_create()
{
    return itembox_create(collectiblebox_strategy, 3);
}

item_t* starbox_create()
{
    return itembox_create(starbox_strategy, 4);
}

item_t* speedbox_create()
{ 
   return itembox_create(speedbox_strategy, 5);
}

item_t* glassesbox_create()
{
    return itembox_create(glassesbox_strategy, 6);
}

item_t* shieldbox_create()
{
    return itembox_create(shieldbox_strategy, 7);
}

item_t* trapbox_create()
{
    return itembox_create(trapbox_strategy, 8);
}

item_t* emptybox_create()
{
    return itembox_create(emptybox_strategy, 9);
}

item_t* fireshieldbox_create()
{
    return itembox_create(fireshieldbox_strategy, 11);
}

item_t* thundershieldbox_create()
{
    return itembox_create(thundershieldbox_strategy, 12);
}

item_t* watershieldbox_create()
{
    return itembox_create(watershieldbox_strategy, 13);
}

item_t* acidshieldbox_create()
{
    return itembox_create(acidshieldbox_strategy, 14);
}

item_t* windshieldbox_create()
{
    return itembox_create(windshieldbox_strategy, 15);
}


/* private strategies */
void lifebox_strategy(item_t *item, player_t *player)
{
    level_add_to_score(100);
    player_set_lives( player_get_lives()+1 );
    level_override_music( soundfactory_get("1up") );
}

void collectiblebox_strategy(item_t *item, player_t *player)
{
    level_add_to_score(100);
    player_set_collectibles( player_get_collectibles()+10 );
    sound_play( soundfactory_get("ring") );
}

void starbox_strategy(item_t *item, player_t *player)
{
    level_add_to_score(100);
    player->invincible = TRUE;
    player->invtimer = 0;
    music_play( music_load("musics/invincible.ogg"), 0 );
}

void speedbox_strategy(item_t *item, player_t *player)
{
    level_add_to_score(100);
    player->got_speedshoes = TRUE;
    player->speedshoes_timer = 0;
    music_play( music_load("musics/speed.ogg"), 0 );
}

void glassesbox_strategy(item_t *item, player_t *player)
{
    level_add_to_score(100);
    player->got_glasses = TRUE;
}

void shieldbox_strategy(item_t *item, player_t *player)
{
    level_add_to_score(100);
    player->shield_type = SH_SHIELD;
    sound_play( soundfactory_get("shield") );
}

void fireshieldbox_strategy(item_t *item, player_t *player)
{
    level_add_to_score(100);
    player->shield_type = SH_FIRESHIELD;
    sound_play( soundfactory_get("fire shield") );
}

void thundershieldbox_strategy(item_t *item, player_t *player)
{
    level_add_to_score(100);
    player->shield_type = SH_THUNDERSHIELD;
    sound_play( soundfactory_get("thunder shield") );
}

void watershieldbox_strategy(item_t *item, player_t *player)
{
    level_add_to_score(100);
    player->shield_type = SH_WATERSHIELD;
    sound_play( soundfactory_get("water shield") );
}

void acidshieldbox_strategy(item_t *item, player_t *player)
{
    level_add_to_score(100);
    player->shield_type = SH_ACIDSHIELD;
    sound_play( soundfactory_get("acid shield") );
}

void windshieldbox_strategy(item_t *item, player_t *player)
{
    level_add_to_score(100);
    player->shield_type = SH_WINDSHIELD;
    sound_play( soundfactory_get("wind shield") );
}

void trapbox_strategy(item_t *item, player_t *player)
{
    player_hit(player, item->actor);
}

void emptybox_strategy(item_t *item, player_t *player)
{
    level_add_to_score(100);
}


/* private methods */
item_t* itembox_create(void (*on_destroy)(item_t*,player_t*), int anim_id)
{
    item_t *item = mallocx(sizeof(itembox_t));
    itembox_t *me = (itembox_t*)item;

    item->init = itembox_init;
    item->release = itembox_release;
    item->update = itembox_update;
    item->render = itembox_render;

    me->on_destroy = on_destroy;
    me->anim_id = anim_id;

    return item;
}

void itembox_init(item_t *item)
{
    itembox_t *me = (itembox_t*)item;

    item->always_active = FALSE;
    item->obstacle = TRUE;
    item->bring_to_back = FALSE;
    item->preserve = TRUE;
    item->actor = actor_create();

    actor_change_animation(item->actor, sprite_get_animation("SD_ITEMBOX", me->anim_id));
}

void itembox_release(item_t* item)
{
    actor_destroy(item->actor);
}

void itembox_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    int i;
    actor_t *act = item->actor;
    itembox_t *me = (itembox_t*)item;

    item->obstacle = !player_is_attacking( level_player() );

    for(i=0; i<team_size; i++) {
        player_t *player = team[i];

        /* the player is about to crash this box... */
        if(item->state == IS_IDLE && actor_pixelperfect_collision(item->actor, player->actor) && player_is_attacking(player)) {
            item_t *icon = level_create_item(IT_ICON, v2d_add(act->position, v2d_new(0,-5)));
            icon_change_animation(icon, me->anim_id);
            level_create_item(IT_EXPLOSION, v2d_add(act->position, v2d_new(0,-20)));
            level_create_item(IT_CRUSHEDBOX, act->position);

            sound_play( soundfactory_get("destroy") );
            player_bounce(player, act);

            me->on_destroy(item, player);
            item->state = IS_DEAD;
        }
    }

    /* animation */
    me->anim_id = me->anim_id < 3 ? get_anim_id( level_player()->name ) : me->anim_id;
    actor_change_animation(item->actor, sprite_get_animation("SD_ITEMBOX", me->anim_id));
}

void itembox_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}


int get_anim_id(const char *player_name)
{
    if(str_icmp(player_name, "Surge") == 0)
        return 0;
    else if(str_icmp(player_name, "Neon") == 0)
        return 1;
    else if(str_icmp(player_name, "Charge") == 0)
        return 2;
    else
        return 0;
}
