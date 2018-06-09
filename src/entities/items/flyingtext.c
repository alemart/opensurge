/*
 * Open Surge Engine
 * flyingtext.c - flying text
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

#include "flyingtext.h"
#include "../../core/util.h"
#include "../../core/timer.h"
#include "../../core/font.h"
#include "../player.h"
#include "../brick.h"
#include "../item.h"
#include "../enemy.h"
#include "../actor.h"

/* flyingtext class */
typedef struct flyingtext_t flyingtext_t;
struct flyingtext_t {
    item_t item; /* base class */
    font_t *font;
    float elapsed_time;
    v2d_t textsize;
};

static void flyingtext_init(item_t *item);
static void flyingtext_release(item_t* item);
static void flyingtext_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void flyingtext_render(item_t* item, v2d_t camera_position);



/* public methods */
item_t* flyingtext_create()
{
    item_t *item = mallocx(sizeof(flyingtext_t));

    item->init = flyingtext_init;
    item->release = flyingtext_release;
    item->update = flyingtext_update;
    item->render = flyingtext_render;

    return item;
}

void flyingtext_set_text(item_t *item, const char *text)
{
    flyingtext_t *me = (flyingtext_t*)item;
    font_set_text(me->font, "%s", text);
    me->textsize = font_get_textsize(me->font);
}


/* private methods */
void flyingtext_init(item_t *item)
{
    flyingtext_t *me = (flyingtext_t*)item;

    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = FALSE;
    item->preserve = FALSE;
    item->actor = actor_create();

    me->elapsed_time = 0.0f;
    me->font = font_create("default");
    flyingtext_set_text(item, "0");

    actor_change_animation(item->actor, sprite_get_animation("SD_QUESTIONMARK", 0));
    item->actor->visible = FALSE;
}



void flyingtext_release(item_t* item)
{
    flyingtext_t *me = (flyingtext_t*)item;

    actor_destroy(item->actor);
    font_destroy(me->font);
}



void flyingtext_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    flyingtext_t *me = (flyingtext_t*)item;
    float dt = timer_get_delta();

    me->elapsed_time += dt;
    if(me->elapsed_time < 0.5f)
        item->actor->position.y -= 100.0f * dt;
    else if(me->elapsed_time > 2.0f)
        item->state = IS_DEAD;

    font_set_position(me->font, v2d_subtract(item->actor->position, v2d_new(me->textsize.x/2, me->textsize.y/2)));
}


void flyingtext_render(item_t* item, v2d_t camera_position)
{
    flyingtext_t *me = (flyingtext_t*)item;
    font_render(me->font, camera_position);
}

