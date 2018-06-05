/*
 * Open Surge Engine
 * switch.c - teleporter/regular door switch
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

#include "switch.h"
#include "door.h"
#include "teleporter.h"
#include "util/itemutil.h"
#include "../../core/v2d.h"
#include "../../core/util.h"
#include "../../core/audio.h"
#include "../../core/video.h"
#include "../../core/image.h"
#include "../../core/soundfactory.h"
#include "../../scenes/level.h"
#include "../player.h"
#include "../brick.h"
#include "../item.h"
#include "../enemy.h"
#include "../actor.h"

/* switch class */
typedef struct switch_t switch_t;
struct switch_t {
    item_t item; /* base class */
    int is_pressed; /* is this switch being pressed? */
    item_t *partner; /* the object I am coupled with (may be NULL, a door or a teleporter) */
};

static void switch_init(item_t *item);
static void switch_release(item_t* item);
static void switch_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void switch_render(item_t* item, v2d_t camera_position);

static int pressed_the_switch(item_t *item, player_t *player);
static void handle_logic(item_t *item, item_t *other, player_t **team, int team_size, void (*stepin)(item_t*,player_t*), void (*stepout)(item_t*));

static void stepin_nothing(item_t *door, player_t *who);
static void stepout_nothing(item_t *door);
static void stepin_door(item_t *door, player_t *who);
static void stepout_door(item_t *door);
static void stepin_teleporter(item_t *teleporter, player_t *who);
static void stepout_teleporter(item_t *teleporter);



/* public methods */
item_t* switch_create()
{
    item_t *item = mallocx(sizeof(switch_t));

    item->init = switch_init;
    item->release = switch_release;
    item->update = switch_update;
    item->render = switch_render;

    return item;
}


/* private methods */
void switch_init(item_t *item)
{
    switch_t *me = (switch_t*)item;

    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    me->is_pressed = FALSE;
    me->partner = NULL;
    actor_change_animation(item->actor, sprite_get_animation("SD_SWITCH", 0));
}



void switch_release(item_t* item)
{
    actor_destroy(item->actor);
}



void switch_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    switch_t *me = (switch_t*)item;
    item_t *door, *teleporter;
    float d1, d2;

    /* I have no partner */
    me->partner = NULL;

    /* figuring out who is my partner */
    door = find_closest_item(item, item_list, IT_DOOR, &d1);
    teleporter = find_closest_item(item, item_list, IT_TELEPORTER, &d2);
    if(door != NULL && d1 < d2)
        me->partner = door;
    if(teleporter != NULL && d2 < d1)
        me->partner = teleporter;

    /* handle the logic. Which logic? That depends. Who is my partner, if any? */
    if(me->partner == NULL)
        handle_logic(item, door, team, team_size, stepin_nothing, stepout_nothing);
    else if(me->partner == door)
        handle_logic(item, door, team, team_size, stepin_door, stepout_door);
    else if(me->partner == teleporter)
        handle_logic(item, teleporter, team, team_size, stepin_teleporter, stepout_teleporter);
}


void switch_render(item_t* item, v2d_t camera_position)
{
    switch_t *me = (switch_t*)item;

    if(level_editmode() && me->partner != NULL) {
        v2d_t p1, p2, offset;
        offset = v2d_subtract(camera_position, v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2));
        p1 = v2d_subtract(item->actor->position, offset);
        p2 = v2d_subtract(me->partner->actor->position, offset);
        image_line(video_get_backbuffer(), (int)p1.x, (int)p1.y, (int)p2.x, (int)p2.y, image_rgb(255, 0, 0));
    }

    actor_render(item->actor, camera_position);
}



/* misc */
void handle_logic(item_t *item, item_t *other, player_t **team, int team_size, void (*stepin)(item_t*,player_t*), void (*stepout)(item_t*))
{
    int i;
    int nobody_is_pressing_me = TRUE;
    switch_t *me = (switch_t*)item;
    actor_t *act = item->actor;

    /* step in */
    for(i=0; i<team_size; i++) {
        player_t *player = team[i];

        if(pressed_the_switch(item, player)) {
            nobody_is_pressing_me = FALSE;
            if(!me->is_pressed) {
                stepin(other, player);
                sound_play( soundfactory_get("switch") );
                actor_change_animation(act, sprite_get_animation("SD_SWITCH", 1));
                me->is_pressed = TRUE;
            }
        }
    }

    /* step out */
    if(nobody_is_pressing_me) {
        if(me->is_pressed) {
            stepout(other);
            actor_change_animation(act, sprite_get_animation("SD_SWITCH", 0));
            me->is_pressed = FALSE;
        }
    }
}

void stepin_nothing(item_t *door, player_t *who)
{
    ; /* empty */
}

void stepout_nothing(item_t *door)
{
    ; /* empty */
}

void stepin_door(item_t *door, player_t *who)
{
    door_open(door);
}

void stepout_door(item_t *door)
{
    door_close(door);
}

void stepin_teleporter(item_t *teleporter, player_t *who)
{
    teleporter_activate(teleporter, who);
}

void stepout_teleporter(item_t *teleporter)
{
    ; /* empty */
}

/* returns true if the player has pressed the switch (item) */
int pressed_the_switch(item_t *item, player_t *player)
{
    float a[4], b[4];

    a[0] = item->actor->position.x - item->actor->hot_spot.x;
    a[1] = item->actor->position.y - item->actor->hot_spot.y;
    a[2] = a[0] + image_width(actor_image(item->actor));
    a[3] = a[1] + image_height(actor_image(item->actor));

    b[0] = player->actor->position.x - player->actor->hot_spot.x + image_width(actor_image(player->actor)) * 0.3;
    b[1] = player->actor->position.y - player->actor->hot_spot.y + image_height(actor_image(player->actor)) * 0.5;
    b[2] = b[0] + image_width(actor_image(player->actor)) * 0.4;
    b[3] = b[1] + image_height(actor_image(player->actor)) * 0.5;

    return (!player_is_dying(player) && bounding_box(a,b));
}

