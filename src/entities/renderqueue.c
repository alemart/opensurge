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



/* the types of renderables */
enum {
    TYPE_PLAYER,

    TYPE_BRICK,
    TYPE_BRICK_MASK,
    TYPE_PARTICLE,

    TYPE_SSOBJECT,
    TYPE_SSOBJECT_DEBUG,
    TYPE_SSOBJECT_GIZMO,

    TYPE_BACKGROUND,
    TYPE_FOREGROUND,
    TYPE_WATER,

    TYPE_ITEM, /* legacy item */
    TYPE_OBJECT /* legacy object */
};

/* a renderable entity */
typedef union renderable_t renderable_t;
union renderable_t {
    player_t* player;
    brick_t* brick;
    item_t* item;
    object_t* object; /* legacy object */
    surgescript_object_t* ssobject;
    bgtheme_t* theme;
    void* dummy;
};

/* a vtable used for rendering different types of entities */
typedef struct renderable_vtable_t renderable_vtable_t;
struct renderable_vtable_t {
    float (*zindex)(renderable_t);
    void (*render)(renderable_t,v2d_t);
    int (*ypos)(renderable_t);
    int (*type)(renderable_t);
};

/* an entry of the render queue */
typedef struct renderqueue_entry_t renderqueue_entry_t;
struct renderqueue_entry_t {
    renderable_t entity;
    const renderable_vtable_t* vtable;
};

/* vtables */
static float zindex_particles(renderable_t r);
static float zindex_player(renderable_t r);
static float zindex_item(renderable_t r);
static float zindex_object(renderable_t r);
static float zindex_brick(renderable_t r);
static float zindex_brick_mask(renderable_t r);
static float zindex_ssobject(renderable_t r);
static float zindex_ssobject_debug(renderable_t r);
static float zindex_ssobject_gizmo(renderable_t r);
static float zindex_background(renderable_t r);
static float zindex_foreground(renderable_t r);
static float zindex_water(renderable_t r);

static void render_particles(renderable_t r, v2d_t camera_position);
static void render_player(renderable_t r, v2d_t camera_position);
static void render_item(renderable_t r, v2d_t camera_position);
static void render_object(renderable_t r, v2d_t camera_position);
static void render_brick(renderable_t r, v2d_t camera_position);
static void render_brick_mask(renderable_t r, v2d_t camera_position);
static void render_ssobject(renderable_t r, v2d_t camera_position);
static void render_ssobject_gizmo(renderable_t r, v2d_t camera_position);
static void render_ssobject_debug(renderable_t r, v2d_t camera_position);
static void render_background(renderable_t r, v2d_t camera_position);
static void render_foreground(renderable_t r, v2d_t camera_position);
static void render_water(renderable_t r, v2d_t camera_position);

static int ypos_particles(renderable_t r);
static int ypos_player(renderable_t r);
static int ypos_item(renderable_t r);
static int ypos_object(renderable_t r);
static int ypos_brick(renderable_t r);
static int ypos_brick_mask(renderable_t r);
static int ypos_ssobject(renderable_t r);
static int ypos_ssobject_debug(renderable_t r);
static int ypos_ssobject_gizmo(renderable_t r);
static int ypos_background(renderable_t r);
static int ypos_foreground(renderable_t r);
static int ypos_water(renderable_t r);

static int type_particles(renderable_t r);
static int type_player(renderable_t r);
static int type_item(renderable_t r);
static int type_object(renderable_t r);
static int type_brick(renderable_t r);
static int type_brick_mask(renderable_t r);
static int type_ssobject(renderable_t r);
static int type_ssobject_debug(renderable_t r);
static int type_ssobject_gizmo(renderable_t r);
static int type_background(renderable_t r);
static int type_foreground(renderable_t r);
static int type_water(renderable_t r);

static const renderable_vtable_t VTABLE[] = {
    [TYPE_BRICK] = {
        .zindex = zindex_brick,
        .render = render_brick,
        .ypos = ypos_brick,
        .type = type_brick
    },

    [TYPE_BRICK_MASK] = {
        .zindex = zindex_brick_mask,
        .render = render_brick_mask,
        .ypos = ypos_brick_mask,
        .type = type_brick_mask
    },

    [TYPE_ITEM] = {
        .zindex = zindex_item,
        .render = render_item,
        .ypos = ypos_item,
        .type = type_item
    },

    [TYPE_OBJECT] = {
        .zindex = zindex_object,
        .render = render_object,
        .ypos = ypos_object,
        .type = type_object
    },

    [TYPE_PLAYER] = {
        .zindex = zindex_player,
        .render = render_player,
        .ypos = ypos_player,
        .type = type_player
    },

    [TYPE_PARTICLE] = {
        .zindex = zindex_particles,
        .render = render_particles,
        .ypos = ypos_particles,
        .type = type_particles
    },

    [TYPE_SSOBJECT] = {
        .zindex = zindex_ssobject,
        .render = render_ssobject,
        .ypos = ypos_ssobject,
        .type = type_ssobject
    },

    [TYPE_SSOBJECT_DEBUG] = {
        .zindex = zindex_ssobject_debug,
        .render = render_ssobject_debug,
        .ypos = ypos_ssobject_debug,
        .type = type_ssobject_debug
    },

    [TYPE_SSOBJECT_GIZMO] = {
        .zindex = zindex_ssobject_gizmo,
        .render = render_ssobject_gizmo,
        .ypos = ypos_ssobject_gizmo,
        .type = type_ssobject_gizmo
    },

    [TYPE_BACKGROUND] = {
        .zindex = zindex_background,
        .render = render_background,
        .ypos = ypos_background,
        .type = type_background
    },

    [TYPE_FOREGROUND] = {
        .zindex = zindex_foreground,
        .render = render_foreground,
        .ypos = ypos_foreground,
        .type = type_foreground
    },

    [TYPE_WATER] = {
        .zindex = zindex_water,
        .render = render_water,
        .ypos = ypos_water,
        .type = type_water
    }
};

/* utilities */
#define ZINDEX_OFFSET(n)          (0.000001f * (float)(n)) /* ZINDEX_OFFSET(1) is the mininum zindex offset */
#define ZINDEX_LARGE              99999.0f /* will be displayed in front of others */
#define INITIAL_BUFFER_CAPACITY   256
static int cmp_fun(const void *i, const void *j);
static inline float brick_zindex_offset(const brick_t *brick);
static void enqueue(const renderqueue_entry_t* entry);

/* internal data */
static renderqueue_entry_t* buffer = NULL;
static int buffer_size = 0;
static int buffer_capacity = 0;
static v2d_t camera;





/* ----- public interface -----*/



/*
 * renderqueue_init()
 * Initializes the render queue
 */
void renderqueue_init()
{
    buffer_size = 0;
    buffer_capacity = INITIAL_BUFFER_CAPACITY;
    buffer = malloc(buffer_capacity * sizeof(*buffer));

    camera = v2d_new(0, 0);
}

/*
 * renderqueue_release()
 * Deinitializes the render queue
 */
void renderqueue_release()
{
    free(buffer);
    buffer_capacity = 0;
    buffer_size = 0;
}




/*
 * renderqueue_begin()
 * Starts a new rendering process
 */
void renderqueue_begin(v2d_t camera_position)
{
    camera = camera_position;
    buffer_size = 0;
}

/*
 * renderqueue_end()
 * Finishes an existing rendering process
 * (will render everything in the queue)
 */
void renderqueue_end()
{
    /* sort the entries with a stable sorting algorithm */
    merge_sort(buffer, buffer_size, sizeof(*buffer), cmp_fun);

    /* render the entries */
    for(int i = 0; i < buffer_size; i++)
        buffer[i].vtable->render(buffer[i].entity, camera);

    /* clean up */
    buffer_size = 0;
}




/*
 * renderqueue_enqueue_brick()
 * Enqueues a brick
 */
void renderqueue_enqueue_brick(brick_t *brick)
{
    renderqueue_entry_t entry = {
        .entity.brick = brick,
        .vtable = &VTABLE[TYPE_BRICK]
    };

    enqueue(&entry);
}

/*
 * renderqueue_enqueue_brick_mask()
 * Enqueues a brick mask
 */
void renderqueue_enqueue_brick_mask(brick_t *brick)
{
    renderqueue_entry_t entry = {
        .entity.brick = brick,
        .vtable = &VTABLE[TYPE_BRICK_MASK]
    };

    enqueue(&entry);
}

/*
 * renderqueue_enqueue_item()
 * Enqueues a legacy item
 */
void renderqueue_enqueue_item(item_t *item)
{
    renderqueue_entry_t entry = {
        .entity.item = item,
        .vtable = &VTABLE[TYPE_ITEM]
    };

    enqueue(&entry);
}

/*
 * renderqueue_enqueue_object()
 * Enqueues a legacy object
 */
void renderqueue_enqueue_object(object_t *object)
{
    renderqueue_entry_t entry = {
        .entity.object = object,
        .vtable = &VTABLE[TYPE_OBJECT]
    };

    enqueue(&entry);
}

/*
 * renderqueue_enqueue_player()
 * Enqueues a player
 */
void renderqueue_enqueue_player(player_t *player)
{
    renderqueue_entry_t entry = {
        .entity.player = player,
        .vtable = &VTABLE[TYPE_PLAYER]
    };

    enqueue(&entry);
}

/*
 * renderqueue_enqueue_particles()
 * Enqueues the level particles
 */
void renderqueue_enqueue_particles()
{
    renderqueue_entry_t entry = {
        .entity.dummy = NULL,
        .vtable = &VTABLE[TYPE_PARTICLE]
    };

    enqueue(&entry);
}

/*
 * renderqueue_enqueue_ssobject()
 * Enqueues a SurgeScript object
 */
void renderqueue_enqueue_ssobject(surgescript_object_t* object)
{
    renderqueue_entry_t entry = {
        .entity.ssobject = object,
        .vtable = &VTABLE[TYPE_SSOBJECT]
    };

    enqueue(&entry);
}

/*
 * renderqueue_enqueue_ssobject_debug()
 * Enqueues a SurgeScript object (editor)
 */
void renderqueue_enqueue_ssobject_debug(surgescript_object_t* object)
{
    renderqueue_entry_t entry = {
        .entity.ssobject = object,
        .vtable = &VTABLE[TYPE_SSOBJECT_DEBUG]
    };

    enqueue(&entry);
}

/*
 * renderqueue_enqueue_ssobject_gizmo()
 * Enqueues a SurgeScript object gizmo (editor)
 */
void renderqueue_enqueue_ssobject_gizmo(surgescript_object_t* object)
{
    renderqueue_entry_t entry = {
        .entity.ssobject = object,
        .vtable = &VTABLE[TYPE_SSOBJECT_GIZMO]
    };

    enqueue(&entry);
}

/*
 * renderqueue_enqueue_background()
 * Enqueues the background
 */
void renderqueue_enqueue_background(bgtheme_t* background)
{
    renderqueue_entry_t entry = {
        .entity.theme = background,
        .vtable = &VTABLE[TYPE_BACKGROUND]
    };

    enqueue(&entry);
}

/*
 * renderqueue_enqueue_foreground()
 * Enqueues the foreground
 */
void renderqueue_enqueue_foreground(bgtheme_t* foreground)
{
    renderqueue_entry_t entry = {
        .entity.theme = foreground,
        .vtable = &VTABLE[TYPE_FOREGROUND]
    };

    enqueue(&entry);
}

/*
 * renderqueue_enqueue_water()
 * Enqueues the water
 */
void renderqueue_enqueue_water()
{
    renderqueue_entry_t entry = {
        .entity.dummy = NULL,
        .vtable = &VTABLE[TYPE_WATER]
    };

    enqueue(&entry);
}





/* ----- private utilities ----- */

/* enqueues an entry */
void enqueue(const renderqueue_entry_t* entry)
{
    /* grow the buffer if necessary */
    if(buffer_size == buffer_capacity) {
        buffer_capacity *= 2;
        buffer = realloc(buffer, buffer_capacity * sizeof(*buffer));
    }

    /* add the entry to the buffer */
    buffer[buffer_size++] = *entry;
}

/* compares two entries of the render queue */
int cmp_fun(const void *i, const void *j)
{
    const renderqueue_entry_t *a = (const renderqueue_entry_t*)i;
    const renderqueue_entry_t *b = (const renderqueue_entry_t*)j;
    float za = a->vtable->zindex(a->entity);
    float zb = b->vtable->zindex(b->entity);

    if(fabs(za - zb) * 10.0f < ZINDEX_OFFSET(1)) {
        if(a->vtable->type(a->entity) == b->vtable->type(b->entity))
            return a->vtable->ypos(a->entity) - b->vtable->ypos(b->entity);
        else
            return (a->vtable->type(a->entity) == TYPE_PLAYER) - (b->vtable->type(b->entity) == TYPE_PLAYER);
    }

    return (za > zb) - (za < zb);
}

/* compute a tiny zindex offset for a brick depending on its type, layer and behavior */
float brick_zindex_offset(const brick_t *brick)
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






/* ----- private strategies ----- */

int type_particles(renderable_t r) { return TYPE_PARTICLE; }
int type_player(renderable_t r) { return TYPE_PLAYER; }
int type_item(renderable_t r) { return TYPE_ITEM; }
int type_object(renderable_t r) { return TYPE_OBJECT; }
int type_brick(renderable_t r) { return TYPE_BRICK; }
int type_brick_mask(renderable_t r) { return TYPE_BRICK_MASK; }
int type_ssobject(renderable_t r) { return TYPE_SSOBJECT; }
int type_ssobject_debug(renderable_t r) { return TYPE_SSOBJECT_DEBUG; }
int type_ssobject_gizmo(renderable_t r) { return TYPE_SSOBJECT_GIZMO; }
int type_background(renderable_t r) { return TYPE_BACKGROUND; }
int type_foreground(renderable_t r) { return TYPE_FOREGROUND; }
int type_water(renderable_t r) { return TYPE_WATER; }

float zindex_particles(renderable_t r) { return 1.0f; }
float zindex_player(renderable_t r) { return player_is_dying(r.player) ? (1.0f - ZINDEX_OFFSET(1)) : 0.5f; }
float zindex_item(renderable_t r) { return 0.5f - (r.item->bring_to_back ? ZINDEX_OFFSET(1) : 0.0f); }
float zindex_object(renderable_t r) { return r.object->zindex; }
float zindex_brick(renderable_t r) { return brick_zindex(r.brick) + brick_zindex_offset(r.brick); }
float zindex_brick_mask(renderable_t r) { return ZINDEX_LARGE + brick_zindex_offset(r.brick); }
float zindex_ssobject(renderable_t r) { return scripting_util_object_zindex(r.ssobject); }
float zindex_ssobject_debug(renderable_t r) { return scripting_util_object_zindex(r.ssobject); } /* TODO: check children */
float zindex_ssobject_gizmo(renderable_t r) { return ZINDEX_LARGE; }
float zindex_background(renderable_t r) { return 0.0f; }
float zindex_foreground(renderable_t r) { return 1.0f; }
float zindex_water(renderable_t r) { return 1.0f; }

int ypos_particles(renderable_t r) { return 0; }
int ypos_player(renderable_t r) { return 0; } /*(int)(r.player->actor->position.y);*/
int ypos_item(renderable_t r) { return (int)(r.item->actor->position.y); }
int ypos_object(renderable_t r) { return (int)(r.object->actor->position.y); }
int ypos_brick(renderable_t r) { return brick_position(r.brick).y; }
int ypos_brick_mask(renderable_t r) { return ypos_brick(r); }
int ypos_ssobject(renderable_t r) { return 0; } /* TODO (not needed?) */
int ypos_ssobject_debug(renderable_t r) { return ypos_ssobject(r); }
int ypos_ssobject_gizmo(renderable_t r) { return ypos_ssobject(r); }
int ypos_background(renderable_t r) { return 0; } /* preserve relative indexes */
int ypos_foreground(renderable_t r) { return 0; } /* preserve relative indexes */
int ypos_water(renderable_t r) { return 0; } /* not needed */


/* --- private rendering routines --- */

void render_particles(renderable_t r, v2d_t camera_position)
{
    particle_render(camera_position);
}

void render_player(renderable_t r, v2d_t camera_position)
{
    player_render(r.player, camera_position);
}

void render_item(renderable_t r, v2d_t camera_position)
{
    item_render(r.item, camera_position);
}

void render_object(renderable_t r, v2d_t camera_position)
{
    enemy_render(r.object, camera_position);
}

void render_brick(renderable_t r, v2d_t camera_position)
{
    brick_render(r.brick, camera_position);
}

void render_brick_mask(renderable_t r, v2d_t camera_position)
{
    brick_render_mask(r.brick, camera_position);
}

void render_ssobject(renderable_t r, v2d_t camera_position)
{
    surgescript_object_call_function(r.ssobject, "onRender", NULL, 0, NULL);
}

void render_ssobject_gizmo(renderable_t r, v2d_t camera_position)
{
    surgescript_object_call_function(r.ssobject, "onRenderGizmos", NULL, 0, NULL);
}

void render_ssobject_debug(renderable_t r, v2d_t camera_position)
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

void render_background(renderable_t r, v2d_t camera_position)
{
    background_render_bg(r.theme, camera_position);
}

void render_foreground(renderable_t r, v2d_t camera_position)
{
    background_render_fg(r.theme, camera_position);
}

void render_water(renderable_t r, v2d_t camera_position)
{
    /* convert the waterlevel from world space to screen space */
    int y = level_waterlevel() - ((int)camera_position.y - VIDEO_SCREEN_H / 2);

    /* clip out */
    if(y >= VIDEO_SCREEN_H)
        return;

    /* adjust y */
    y = max(0, y);

    /*

    Let's adjust the color of the water by pre-multiplying the alpha value

    "By default Allegro uses pre-multiplied alpha for transparent blending of
    bitmaps and primitives (see al_load_bitmap_flags for a discussion of that
    feature). This means that if you want to tint a bitmap or primitive to be
    transparent you need to multiply the color components by the alpha
    components when you pass them to this function."

    Source: Allegro manual at
    https://liballeg.org/a5docs/trunk/graphics.html#al_premul_rgba

    */
    uint8_t red, green, blue, alpha;
    color_t color = level_watercolor();
    color_unmap(color, &red, &green, &blue, &alpha);
    color = color_premul_rgba(red, green, blue, alpha);

    /* render the water */
    image_rectfill(0, y, VIDEO_SCREEN_W, VIDEO_SCREEN_H, color);
}