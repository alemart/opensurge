/*
 * Open Surge Engine
 * dnadoor.c - DNA doors
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

#include "dnadoor.h"
#include "../../core/util.h"
#include "../../core/stringutil.h"
#include "../../core/timer.h"
#include "../player.h"
#include "../brick.h"
#include "../item.h"
#include "../enemy.h"
#include "../actor.h"

/* dnadoor class */
typedef struct dnadoor_t dnadoor_t;
struct dnadoor_t {
    item_t item; /* base class */
    char *authorized_player_name;
    int is_vertical_door;
};

static item_t *dnadoor_create(const char *authorized_player_name, int is_vertical_door);
static void dnadoor_init(item_t *item);
static void dnadoor_release(item_t* item);
static void dnadoor_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void dnadoor_render(item_t* item, v2d_t camera_position);

static int hittest(player_t *player, item_t *dnadoor);


/* public methods */
item_t* surge_dnadoor_create()
{
    return dnadoor_create("Surge", TRUE);
}

item_t* neon_dnadoor_create()
{
    return dnadoor_create("Neon", TRUE);
}

item_t* charge_dnadoor_create()
{
    return dnadoor_create("Charge", TRUE);
}

item_t* surge_horizontal_dnadoor_create()
{
    return dnadoor_create("Surge", FALSE);
}

item_t* neon_horizontal_dnadoor_create()
{
    return dnadoor_create("Neon", FALSE);
}

item_t* charge_horizontal_dnadoor_create()
{
    return dnadoor_create("Charge", FALSE);
}


/* private methods */
item_t* dnadoor_create(const char *authorized_player_name, int is_vertical_door)
{
    item_t *item = mallocx(sizeof(dnadoor_t));
    dnadoor_t *me = (dnadoor_t*)item;

    item->init = dnadoor_init;
    item->release = dnadoor_release;
    item->update = dnadoor_update;
    item->render = dnadoor_render;

    me->authorized_player_name = str_dup(authorized_player_name);
    me->is_vertical_door = is_vertical_door;

    return item;
}


void dnadoor_init(item_t *item)
{
    dnadoor_t *me = (dnadoor_t*)item;
    char *sprite_name;
    int anim_id = 0;

    item->always_active = FALSE;
    item->obstacle = TRUE;
    item->bring_to_back = FALSE;
    item->preserve = TRUE;
    item->actor = actor_create();

    if(str_icmp(me->authorized_player_name, "Surge") == 0)
        anim_id = 0;
    else if(str_icmp(me->authorized_player_name, "Neon") == 0)
        anim_id = 1;
    else if(str_icmp(me->authorized_player_name, "Charge") == 0)
        anim_id = 2;

    sprite_name = me->is_vertical_door ? "SD_DNADOOR" : "SD_HORIZONTALDNADOOR";
    actor_change_animation(item->actor, sprite_get_animation(sprite_name, anim_id));
    actor_synchronize_animation(item->actor, TRUE);
}



void dnadoor_release(item_t* item)
{
    dnadoor_t *me = (dnadoor_t*)item;
    free(me->authorized_player_name);
    actor_destroy(item->actor);
}



void dnadoor_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    dnadoor_t *me = (dnadoor_t*)item;
    actor_t *act = item->actor;
    item_list_t *it;
    int block_anyway = FALSE;
    int collision = FALSE;
    float dt = timer_get_delta();
    float a[4], b[4], diff=5;
    int i;

    /* should we block the DNA Door? */
    item->obstacle = TRUE;
    for(i=0; i<team_size; i++) {
        player_t *player = team[i];
        if(!player_is_dying(player) && hittest(player, item)) {
            if(str_icmp(player->name, me->authorized_player_name) == 0) {
                item->obstacle = FALSE;
                collision = player_collision(player, act);
            }
            else
                block_anyway = TRUE;
        }
    }
    if(block_anyway)
        item->obstacle = TRUE;

    /* cute effect */
    if(item->obstacle)
        act->alpha = min(1.0f, act->alpha + 2.0f * dt);
    else if(collision)
        act->alpha = max(0.4f, act->alpha - 2.0f * dt);

    /* cute effect propagation */
    if(collision) {
        a[0] = act->position.x - act->hot_spot.x - diff;
        a[1] = act->position.y - act->hot_spot.y - diff;
        a[2] = a[0] + image_width(actor_image(act)) + 2*diff;
        a[3] = a[1] + image_height(actor_image(act)) + 2*diff;
        for(it = item_list; it != NULL; it = it->next) {
            if(it->data->type == item->type) {
                b[0] = it->data->actor->position.x - it->data->actor->hot_spot.x - diff;
                b[1] = it->data->actor->position.y - it->data->actor->hot_spot.y - diff;
                b[2] = b[0] + image_width(actor_image(it->data->actor)) + 2*diff;
                b[3] = b[1] + image_height(actor_image(it->data->actor)) + 2*diff;
                if(bounding_box(a,b)) {
                    if(it->data->actor->alpha < act->alpha)
                        act->alpha = it->data->actor->alpha;
                    else
                        it->data->actor->alpha = act->alpha;
                }
            }
        }
    }
}


void dnadoor_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}


/* misc */

/*
as the bounding boxes must a bit larger than usual,
we use some fancy logic to test the collision between
the player and the DNA door.
we don't use a regular actor_collision() test here.
*/
int hittest(player_t *player, item_t *dnadoor)
{
    float a[4], b[4];
    int offset = 3;
    actor_t *pl = player->actor;
    actor_t *act = dnadoor->actor;

    a[0] = pl->position.x - pl->hot_spot.x;
    a[1] = pl->position.y - pl->hot_spot.y;
    a[2] = a[0] + image_width(actor_image(pl));
    a[3] = a[1] + image_height(actor_image(pl));

    b[0] = act->position.x - act->hot_spot.x;
    b[1] = act->position.y - act->hot_spot.y - offset;
    b[2] = b[0] + image_width(actor_image(act));
    b[3] = b[1] + image_height(actor_image(act)) + offset;

    return bounding_box(a, b);
}

