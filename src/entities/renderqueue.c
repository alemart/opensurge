/*
 * Open Surge Engine
 * renderqueue.c - render queue
 * Copyright (C) 2008-2022  Alexandre Martins <alemartf@gmail.com>
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

#include <surgescript.h>
#include <math.h>
#include "renderqueue.h"
#include "particle.h"
#include "player.h"
#include "brick.h"
#include "actor.h"
#include "background.h"
#include "legacy/item.h"
#include "legacy/enemy.h"
#include "../core/util.h"
#include "../core/video.h"
#include "../core/image.h"
#include "../scenes/level.h"
#include "../scripting/scripting.h"

/* private stuff ;) */
typedef union renderable_t renderable_t;
union renderable_t {
    player_t *player;
    brick_t *brick;
    item_t *item;
    object_t *object; /* legacy object */
    surgescript_object_t *ssobject;
    bgtheme_t *theme;
};

typedef struct renderqueue_cell_t renderqueue_cell_t;
struct renderqueue_cell_t {
    renderable_t entity;
    float (*zindex)(renderable_t);
    void (*render)(renderable_t,v2d_t);
    int (*ypos)(renderable_t);
    int (*type)(renderable_t);
};

typedef struct renderqueue_t renderqueue_t;
struct renderqueue_t {
    renderqueue_cell_t cell;
    renderqueue_t *next; /* linked list */
};

static renderqueue_t* queue = NULL;
static int size = 0;
static v2d_t camera;

/* utilities */
#define ZINDEX_OFFSET(n) (0.000001f * (float)(n)) /* ZINDEX_OFFSET(1) is the mininum offset */
#define ZINDEX_LARGE     (99999.0f) /* will be displayed in front of others */
enum { TYPE_PARTICLE, TYPE_PLAYER, TYPE_ITEM, TYPE_OBJECT, TYPE_BRICK, TYPE_SSOBJECT, TYPE_BACKGROUND, TYPE_FOREGROUND, TYPE_WATER };

static int cmp_fun(const void *i, const void *j)
{
    const renderqueue_cell_t *a = (const renderqueue_cell_t*)i;
    const renderqueue_cell_t *b = (const renderqueue_cell_t*)j;
    float za = a->zindex(a->entity), zb = b->zindex(b->entity);

    if(fabs(za - zb) * 10.0f < ZINDEX_OFFSET(1)) {
        if(a->type(a->entity) == b->type(b->entity))
            return a->ypos(a->entity) - b->ypos(b->entity);
        else
            return (a->type(a->entity) == TYPE_PLAYER) - (b->type(b->entity) == TYPE_PLAYER);
    }
    else
        return (za > zb) - (za < zb);
}

static inline float brick_zindex_offset(const brick_t *brick)
{
    float s = 0.0f;

    /* a hackish solution... */
    switch(brick_type(brick)) {
        case BRK_PASSABLE:  s -= ZINDEX_OFFSET(20); break;
        case BRK_CLOUD:     s -= ZINDEX_OFFSET(10); break;
        case BRK_SOLID:     break;
    }

    switch(brick_layer(brick)) {
        case BRL_YELLOW:    s -= ZINDEX_OFFSET(50); break;
        case BRL_GREEN:     s += ZINDEX_OFFSET(50); break; /* |layer offset| > max |type offset| */
        case BRL_DEFAULT:   break;
    }

    /* static bricks should appear behind moving bricks
       if they share the same zindex, type and layer */
    if(brick_behavior(brick) == BRB_DEFAULT)
        s -= ZINDEX_OFFSET(1);
    
    /* done */
    return s;
}

/* private strategies */
static float zindex_particles(renderable_t r) { return 1.0f; }
static float zindex_player(renderable_t r) { return player_is_dying(r.player) ? (1.0f - ZINDEX_OFFSET(1)) : 0.5f; }
static float zindex_item(renderable_t r) { return 0.5f - (r.item->bring_to_back ? ZINDEX_OFFSET(1) : 0.0f); }
static float zindex_object(renderable_t r) { return r.object->zindex; }
static float zindex_brick(renderable_t r) { return brick_zindex(r.brick) + brick_zindex_offset(r.brick); }
static float zindex_brick_mask(renderable_t r) { return ZINDEX_LARGE + brick_zindex_offset(r.brick); }
static float zindex_ssobject(renderable_t r) { return scripting_util_object_zindex(r.ssobject); }
static float zindex_ssobject_debug(renderable_t r) { return scripting_util_object_zindex(r.ssobject); } /* TODO: check children */
static float zindex_ssobject_gizmo(renderable_t r) { return ZINDEX_LARGE; }
static float zindex_background(renderable_t r) { return 0.0f; }
static float zindex_foreground(renderable_t r) { return 1.0f; }
static float zindex_water(renderable_t r) { return 1.0f; }

static void render_particles(renderable_t r, v2d_t camera_position) { particle_render(camera_position); }
static void render_player(renderable_t r, v2d_t camera_position) { player_render(r.player, camera_position); }
static void render_item(renderable_t r, v2d_t camera_position) { item_render(r.item, camera_position); }
static void render_object(renderable_t r, v2d_t camera_position) { enemy_render(r.object, camera_position); }
static void render_brick(renderable_t r, v2d_t camera_position) { brick_render(r.brick, camera_position); }
static void render_brick_mask(renderable_t r, v2d_t camera_position) { brick_render_mask(r.brick, camera_position); }
static void render_ssobject(renderable_t r, v2d_t camera_position) { surgescript_object_call_function(r.ssobject, "onRender", NULL, 0, NULL); }
static void render_ssobject_gizmo(renderable_t r, v2d_t camera_position) { surgescript_object_call_function(r.ssobject, "onRenderGizmos", NULL, 0, NULL); }
static void render_ssobject_debug(renderable_t r, v2d_t camera_position)
{
    /* in render_ssobject_debug(), we don't call the "onRender"
       method of the SurgeScript object, so we don't provoke
       any changes within its state or data */
    const char* name = surgescript_object_name(r.ssobject);
    const animation_t* anim = sprite_animation_exists(name, 0) ? sprite_get_animation(name, 0) : sprite_get_animation(NULL, 0);
    const image_t* img = sprite_get_image(anim, 0);
    v2d_t hot_spot = anim->hot_spot;
    v2d_t position = scripting_util_world_position(r.ssobject);
    if(level_inside_screen(position.x - hot_spot.x, position.y - hot_spot.y, image_width(img), image_height(img))) {
        v2d_t half_screen = v2d_multiply(video_get_screen_size(), 0.5f);
        v2d_t topleft = v2d_subtract(camera_position, half_screen);
        image_draw(img, position.x - hot_spot.x - topleft.x, position.y - hot_spot.y - topleft.y, IF_NONE);
    }
}
static void render_background(renderable_t r, v2d_t camera_position) { background_render_bg(r.theme, camera_position); }
static void render_foreground(renderable_t r, v2d_t camera_position) { background_render_fg(r.theme, camera_position); }
static void render_water(renderable_t r, v2d_t camera_position)
{
    int y = level_waterlevel() - ((int)camera_position.y - VIDEO_SCREEN_H/2);
    if(y < VIDEO_SCREEN_H)
        image_waterfx(y, level_watercolor());
}

static int ypos_particles(renderable_t r) { return 0; }
static int ypos_player(renderable_t r) { return 0; } /*(int)(r.player->actor->position.y);*/
static int ypos_item(renderable_t r) { return (int)(r.item->actor->position.y); }
static int ypos_object(renderable_t r) { return (int)(r.object->actor->position.y); }
static int ypos_brick(renderable_t r) { return brick_position(r.brick).y; }
static int ypos_ssobject(renderable_t r) { return 0; } /* TODO (not needed?) */
static int ypos_background(renderable_t r) { return 0; } /* preserve relative indexes */
static int ypos_foreground(renderable_t r) { return 0; } /* preserve relative indexes */
static int ypos_water(renderable_t r) { return 0; } /* not needed */

static int type_particles(renderable_t r) { return TYPE_PARTICLE; }
static int type_player(renderable_t r) { return TYPE_PLAYER; }
static int type_item(renderable_t r) { return TYPE_ITEM; }
static int type_object(renderable_t r) { return TYPE_OBJECT; }
static int type_brick(renderable_t r) { return TYPE_BRICK; }
static int type_ssobject(renderable_t r) { return TYPE_SSOBJECT; }
static int type_background(renderable_t r) { return TYPE_BACKGROUND; }
static int type_foreground(renderable_t r) { return TYPE_FOREGROUND; }
static int type_water(renderable_t r) { return TYPE_WATER; }




/* public interface */

/* starts a new rendering process */
void renderqueue_begin(v2d_t camera_position)
{
    /* initialize stuff */
    queue = NULL;
    size = 0;
    camera = camera_position;
}

/* finishes an existing rendering process, rendering everything */
void renderqueue_end()
{
    renderqueue_t *it, *next;
    renderqueue_cell_t *arr;
    int i = size;

    /* create a temporary array */
    arr = mallocx(size * sizeof *arr);
    for(it=queue; it; it=it->next)
        arr[--i] = it->cell;

    /* sort stuff. we need an stable sorting algorithm here. */
    merge_sort(arr, size, sizeof *arr, cmp_fun);

    /* render everything */
    for(i=0; i<size; i++)
        arr[i].render(arr[i].entity, camera);

    /* release the temporary array */
    free(arr);

    /* release stuff */
    for(it=queue; it; it=next) {
        next = it->next;
        free(it);
    }
    size = 0;
    queue = NULL;
}

/* enqueues entities */
void renderqueue_enqueue_brick(brick_t *brick)
{
    renderqueue_t *node = mallocx(sizeof *node);
    node->cell.entity.brick = brick;
    node->cell.zindex = zindex_brick;
    node->cell.render = render_brick;
    node->cell.ypos = ypos_brick;
    node->cell.type = type_brick;
    node->next = queue;
    queue = node;
    size++;
}

void renderqueue_enqueue_brick_mask(brick_t *brick)
{
    renderqueue_t *node = mallocx(sizeof *node);
    node->cell.entity.brick = brick;
    node->cell.zindex = zindex_brick_mask;
    node->cell.render = render_brick_mask;
    node->cell.ypos = ypos_brick;
    node->cell.type = type_brick;
    node->next = queue;
    queue = node;
    size++;
}

void renderqueue_enqueue_item(item_t *item)
{
    renderqueue_t *node = mallocx(sizeof *node);
    node->cell.entity.item = item;
    node->cell.zindex = zindex_item;
    node->cell.render = render_item;
    node->cell.ypos = ypos_item;
    node->cell.type = type_item;
    node->next = queue;
    queue = node;
    size++;
}

void renderqueue_enqueue_object(object_t *object)
{
    renderqueue_t *node = mallocx(sizeof *node);
    node->cell.entity.object = object;
    node->cell.zindex = zindex_object;
    node->cell.render = render_object;
    node->cell.ypos = ypos_object;
    node->cell.type = type_object;
    node->next = queue;
    queue = node;
    size++;
}

void renderqueue_enqueue_player(player_t *player)
{
    renderqueue_t *node = mallocx(sizeof *node);
    node->cell.entity.player = player;
    node->cell.zindex = zindex_player;
    node->cell.render = render_player;
    node->cell.ypos = ypos_player;
    node->cell.type = type_player;
    node->next = queue;
    queue = node;
    size++;
}

void renderqueue_enqueue_particles()
{
    renderqueue_t *node = mallocx(sizeof *node);
    node->cell.zindex = zindex_particles;
    node->cell.render = render_particles;
    node->cell.ypos = ypos_particles;
    node->cell.type = type_particles;
    node->next = queue;
    queue = node;
    size++;
}

void renderqueue_enqueue_ssobject(surgescript_object_t* object)
{
    renderqueue_t *node = mallocx(sizeof *node);
    node->cell.entity.ssobject = object;
    node->cell.zindex = zindex_ssobject;
    node->cell.render = render_ssobject;
    node->cell.ypos = ypos_ssobject;
    node->cell.type = type_ssobject;
    node->next = queue;
    queue = node;
    size++;
}

void renderqueue_enqueue_ssobject_debug(surgescript_object_t* object)
{
    renderqueue_t *node = mallocx(sizeof *node);
    node->cell.entity.ssobject = object;
    node->cell.zindex = zindex_ssobject_debug;
    node->cell.render = render_ssobject_debug;
    node->cell.ypos = ypos_ssobject;
    node->cell.type = type_ssobject;
    node->next = queue;
    queue = node;
    size++;
}

void renderqueue_enqueue_ssobject_gizmo(surgescript_object_t* object)
{
    renderqueue_t *node = mallocx(sizeof *node);
    node->cell.entity.ssobject = object;
    node->cell.zindex = zindex_ssobject_gizmo;
    node->cell.render = render_ssobject_gizmo;
    node->cell.ypos = ypos_ssobject;
    node->cell.type = type_ssobject;
    node->next = queue;
    queue = node;
    size++;
}

void renderqueue_enqueue_background(bgtheme_t* background)
{
    renderqueue_t *node = mallocx(sizeof *node);
    node->cell.entity.theme = background;
    node->cell.zindex = zindex_background;
    node->cell.render = render_background;
    node->cell.ypos = ypos_background;
    node->cell.type = type_background;
    node->next = queue;
    queue = node;
    size++;
}

void renderqueue_enqueue_foreground(bgtheme_t* foreground)
{
    renderqueue_t *node = mallocx(sizeof *node);
    node->cell.entity.theme = foreground;
    node->cell.zindex = zindex_foreground;
    node->cell.render = render_foreground;
    node->cell.ypos = ypos_foreground;
    node->cell.type = type_foreground;
    node->next = queue;
    queue = node;
    size++;
}

void renderqueue_enqueue_water()
{
    renderqueue_t *node = mallocx(sizeof *node);
    node->cell.zindex = zindex_water;
    node->cell.render = render_water;
    node->cell.ypos = ypos_water;
    node->cell.type = type_water;
    node->next = queue;
    queue = node;
    size++;
}
