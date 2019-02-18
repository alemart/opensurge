/*
 * Open Surge Engine
 * icon.c - icon
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

#include "icon.h"
#include "../../core/util.h"
#include "../../core/timer.h"
#include "../../scenes/level.h"
#include "../player.h"
#include "../brick.h"
#include "../item.h"
#include "../enemy.h"
#include "../actor.h"

/* icon class */
typedef struct icon_t icon_t;
struct icon_t {
    item_t item; /* base class */
    float elapsed_time; /* elapsed time since this object has been created */
};

static void icon_init(item_t *item);
static void icon_release(item_t* item);
static void icon_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void icon_render(item_t* item, v2d_t camera_position);



/* public methods */
item_t* icon_create()
{
    item_t *item = mallocx(sizeof(icon_t));

    item->init = icon_init;
    item->release = icon_release;
    item->update = icon_update;
    item->render = icon_render;

    return item;
}

void icon_change_animation(item_t *item, int anim_id)
{
    actor_change_animation(item->actor, sprite_get_animation("SD_ICON", anim_id));
}


/* private methods */
void icon_init(item_t *item)
{
    icon_t *me = (icon_t*)item;

    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = FALSE;
    item->preserve = FALSE;
    item->actor = actor_create();

    me->elapsed_time = 0.0f;
    icon_change_animation(item, 0);
}



void icon_release(item_t* item)
{
    actor_destroy(item->actor);
}



void icon_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    icon_t *me = (icon_t*)item;
    actor_t *act = item->actor;
    float dt = timer_get_delta();

    me->elapsed_time += dt;
    if(me->elapsed_time < 1.0f) {
        /* rise */
        act->position.y -= 40.0f * dt;
    }
    else if(me->elapsed_time >= 2.5f) {
        /* death */
#if 1
        item->state = IS_DEAD;
#else
        int i, j;
        int x = (int)(act->position.x-act->hot_spot.x);
        int y = (int)(act->position.y-act->hot_spot.y);
        image_t *img = actor_image(act), *particle;

        /* particle party! :) */
        for(i=0; i<image_height(img); i++) {
            for(j=0; j<image_width(img); j++) {
                particle = image_create(1,1);
                image_clear(particle, image_getpixel(img, j, i));
                level_create_particle(particle, v2d_new(x+j, y+i), v2d_new((j - image_width(img)/2) + (random(image_width(img)) - image_width(img)/2), i - random(image_height(img)/2)), FALSE);
            }
        }

        item->state = IS_DEAD;
#endif
    }
}


void icon_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}

