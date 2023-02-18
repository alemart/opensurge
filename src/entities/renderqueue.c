/*
 * Open Surge Engine
 * renderqueue.c - render queue
 * Copyright (C) 2008-2023  Alexandre Martins <alemartf@gmail.com>
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
#include "../core/stringutil.h"
#include "../core/video.h"
#include "../core/image.h"
#include "../scenes/level.h"
#include "../scripting/scripting.h"



/* the types of renderables */
enum {
    TYPE_PLAYER,

    TYPE_BRICK,
    TYPE_BRICK_MASK,
    TYPE_BRICK_DEBUG,
    TYPE_BRICK_PATH,
    TYPE_PARTICLE,

    TYPE_SSOBJECT,
    TYPE_SSOBJECT_GIZMO,
    TYPE_SSOBJECT_DEBUG,

    TYPE_BACKGROUND,
    TYPE_FOREGROUND,
    TYPE_WATER,

    TYPE_ITEM, /* legacy item */
    TYPE_OBJECT /* legacy object */
};

/* a renderable */
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
    const char* (*path)(renderable_t, char*, size_t);
    int (*type)(renderable_t);
};

/* an entry of the render queue */
typedef struct renderqueue_entry_t renderqueue_entry_t;
struct renderqueue_entry_t {
    renderable_t renderable;
    const renderable_vtable_t* vtable;
    int group_index; /* a helper for deferred rendering; see my commentary about it below */
};

/* vtables */
static float zindex_particles(renderable_t r);
static float zindex_player(renderable_t r);
static float zindex_item(renderable_t r);
static float zindex_object(renderable_t r);
static float zindex_brick(renderable_t r);
static float zindex_brick_mask(renderable_t r);
static float zindex_brick_debug(renderable_t r);
static float zindex_brick_path(renderable_t r);
static float zindex_ssobject(renderable_t r);
static float zindex_ssobject_gizmo(renderable_t r);
static float zindex_ssobject_debug(renderable_t r);
static float zindex_background(renderable_t r);
static float zindex_foreground(renderable_t r);
static float zindex_water(renderable_t r);

static void render_particles(renderable_t r, v2d_t camera_position);
static void render_player(renderable_t r, v2d_t camera_position);
static void render_item(renderable_t r, v2d_t camera_position);
static void render_object(renderable_t r, v2d_t camera_position);
static void render_brick(renderable_t r, v2d_t camera_position);
static void render_brick_mask(renderable_t r, v2d_t camera_position);
static void render_brick_debug(renderable_t r, v2d_t camera_position);
static void render_brick_path(renderable_t r, v2d_t camera_position);
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
static int ypos_brick_debug(renderable_t r);
static int ypos_brick_path(renderable_t r);
static int ypos_ssobject(renderable_t r);
static int ypos_ssobject_gizmo(renderable_t r);
static int ypos_ssobject_debug(renderable_t r);
static int ypos_background(renderable_t r);
static int ypos_foreground(renderable_t r);
static int ypos_water(renderable_t r);

static const char* path_particles(renderable_t r, char* dest, size_t dest_size);
static const char* path_player(renderable_t r, char* dest, size_t dest_size);
static const char* path_item(renderable_t r, char* dest, size_t dest_size);
static const char* path_object(renderable_t r, char* dest, size_t dest_size);
static const char* path_brick(renderable_t r, char* dest, size_t dest_size);
static const char* path_brick_mask(renderable_t r, char* dest, size_t dest_size);
static const char* path_brick_debug(renderable_t r, char* dest, size_t dest_size);
static const char* path_brick_path(renderable_t r, char* dest, size_t dest_size);
static const char* path_ssobject(renderable_t r, char* dest, size_t dest_size);
static const char* path_ssobject_gizmo(renderable_t r, char* dest, size_t dest_size);
static const char* path_ssobject_debug(renderable_t r, char* dest, size_t dest_size);
static const char* path_background(renderable_t r, char* dest, size_t dest_size);
static const char* path_foreground(renderable_t r, char* dest, size_t dest_size);
static const char* path_water(renderable_t r, char* dest, size_t dest_size);

static int type_particles(renderable_t r);
static int type_player(renderable_t r);
static int type_item(renderable_t r);
static int type_object(renderable_t r);
static int type_brick(renderable_t r);
static int type_brick_mask(renderable_t r);
static int type_brick_debug(renderable_t r);
static int type_brick_path(renderable_t r);
static int type_ssobject(renderable_t r);
static int type_ssobject_gizmo(renderable_t r);
static int type_ssobject_debug(renderable_t r);
static int type_background(renderable_t r);
static int type_foreground(renderable_t r);
static int type_water(renderable_t r);

static const renderable_vtable_t VTABLE[] = {
    [TYPE_BRICK] = {
        .zindex = zindex_brick,
        .render = render_brick,
        .ypos = ypos_brick,
        .path = path_brick,
        .type = type_brick
    },

    [TYPE_BRICK_MASK] = {
        .zindex = zindex_brick_mask,
        .render = render_brick_mask,
        .ypos = ypos_brick_mask,
        .path = path_brick_mask,
        .type = type_brick_mask
    },

    [TYPE_BRICK_DEBUG] = {
        .zindex = zindex_brick_debug,
        .render = render_brick_debug,
        .ypos = ypos_brick_debug,
        .path = path_brick_debug,
        .type = type_brick_debug
    },

    [TYPE_BRICK_PATH] = {
        .zindex = zindex_brick_path,
        .render = render_brick_path,
        .ypos = ypos_brick_path,
        .path = path_brick_path,
        .type = type_brick_path
    },

    [TYPE_ITEM] = {
        .zindex = zindex_item,
        .render = render_item,
        .ypos = ypos_item,
        .path = path_item,
        .type = type_item
    },

    [TYPE_OBJECT] = {
        .zindex = zindex_object,
        .render = render_object,
        .ypos = ypos_object,
        .path = path_object,
        .type = type_object
    },

    [TYPE_PLAYER] = {
        .zindex = zindex_player,
        .render = render_player,
        .ypos = ypos_player,
        .path = path_player,
        .type = type_player
    },

    [TYPE_PARTICLE] = {
        .zindex = zindex_particles,
        .render = render_particles,
        .ypos = ypos_particles,
        .path = path_particles,
        .type = type_particles
    },

    [TYPE_SSOBJECT] = {
        .zindex = zindex_ssobject,
        .render = render_ssobject,
        .ypos = ypos_ssobject,
        .path = path_ssobject,
        .type = type_ssobject
    },

    [TYPE_SSOBJECT_DEBUG] = {
        .zindex = zindex_ssobject_debug,
        .render = render_ssobject_debug,
        .ypos = ypos_ssobject_debug,
        .path = path_ssobject_debug,
        .type = type_ssobject_debug
    },

    [TYPE_SSOBJECT_GIZMO] = {
        .zindex = zindex_ssobject_gizmo,
        .render = render_ssobject_gizmo,
        .ypos = ypos_ssobject_gizmo,
        .path = path_ssobject_gizmo,
        .type = type_ssobject_gizmo
    },

    [TYPE_BACKGROUND] = {
        .zindex = zindex_background,
        .render = render_background,
        .ypos = ypos_background,
        .path = path_background,
        .type = type_background
    },

    [TYPE_FOREGROUND] = {
        .zindex = zindex_foreground,
        .render = render_foreground,
        .ypos = ypos_foreground,
        .path = path_foreground,
        .type = type_foreground
    },

    [TYPE_WATER] = {
        .zindex = zindex_water,
        .render = render_water,
        .ypos = ypos_water,
        .path = path_water,
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
static const char* random_path(char prefix);

/* internal data */
static renderqueue_entry_t* buffer = NULL;
static int buffer_size = 0;
static int buffer_capacity = 0;
static v2d_t camera;



/*

OPTIMIZATION: DEFERRED DRAWING
------------------------------

According to the Allegro 5 manual (see the link below), deferred bitmap drawing
"allows for efficient drawing of many bitmaps that share a parent bitmap, such
as sub-bitmaps from a tilesheet or simply identical bitmaps".

In order to optimize the rendering process, we'll group the entries of the
render queue that share the same or a parent bitmap and enable deferred bitmap
drawing whenever we see fit.

Each of the n entries of the render queue is associated with a group_index
defined as follows:

group_index[n-1] = 1

group_index[i] = 1 + group_index[i+1], if path[i] == path[i+1]
                 1 otherwise                               for all 0 <= i < n-1

where path[i] is the path of the image file (if any) of the i-th entry of the
*sorted* render queue. If the paths are the same, then the bitmaps of the two
entries are either the same or one is a descendant of the other, thanks to our
resource manager (see src/core/image.h and src/core/resourcemanager.h). Simply
put, if the paths are the same, we will group the entries.

Let's also define the special off-bounds value group_index[-1] = 1. This is
implemented as a circular array, i.e., group_index[-1] = group_index[n-1] = 1.

It turns out that group_index[] is a piecewise monotonic decreasing sequence:
each piece corresponds to a group.

The optimization is implemented as follows:

for each j = 0 .. n-1,

    1. enable deferred drawing if group_index[j] > group_index[j-1]

    2. draw the j-th entry of the render queue

    3. disable deferred drawing if it's enabled and if group_index[j] == 1

since group_index[n-1] = 1, then deferred drawing will be disabled at the end
of the loop. We must disable deferred drawing because "no drawing is guaranteed
to take place until you disable the hold".

Read the documentation for al_hold_bitmap_drawing() at:
https://liballeg.org/a5docs/trunk/graphics.html#deferred-drawing

*/
#define USE_DEFERRED_DRAWING 1


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
 * Finishes an existing rendering process (will render everything in the queue)
 */
void renderqueue_end()
{
    /* skip if the buffer is empty */
    if(buffer_size == 0)
        return;

    /* sort the entries with a stable sorting algorithm */
    merge_sort(buffer, buffer_size, sizeof(*buffer), cmp_fun);

#if USE_DEFERRED_DRAWING

    /* fill the group_index[] array */
    buffer[buffer_size - 1].group_index = 1;
    for(int i = buffer_size - 2; i >= 0; i--) {
        static char path[2][1024];

        buffer[i].vtable->path(buffer[i].renderable, path[0], sizeof(path[0]));
        buffer[i+1].vtable->path(buffer[i+1].renderable, path[1], sizeof(path[1]));

        if(0 == strcmp(path[0], path[1]))
            buffer[i].group_index = 1 + buffer[i+1].group_index;
        else
            buffer[i].group_index = 1;
    }

    /* render the entries */
    bool held = false;
    for(int j = 0; j < buffer_size; j++) {

        /* enable deferred drawing */
        if(buffer[j].group_index > buffer[(j + (buffer_size - 1)) % buffer_size].group_index) {
            held = true;
            image_hold_drawing(true);
        }

        /* render the j-th entry */
        buffer[j].vtable->render(buffer[j].renderable, camera);

        /* disable deferred drawing */
        if(held && buffer[j].group_index == 1) {
            image_hold_drawing(false);
            held = false;
        }

    }

#else

    /* render the entries without deferred drawing */
    for(int j = 0; j < buffer_size; j++)
        buffer[j].vtable->render(buffer[j].renderable, camera);

#endif

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
        .renderable.brick = brick,
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
        .renderable.brick = brick,
        .vtable = &VTABLE[TYPE_BRICK_MASK]
    };

    enqueue(&entry);
}

/*
 * renderqueue_enqueue_brick_debug()
 * Enqueues a brick (editor)
 */
void renderqueue_enqueue_brick_debug(brick_t *brick)
{
    renderqueue_entry_t entry = {
        .renderable.brick = brick,
        .vtable = &VTABLE[TYPE_BRICK_DEBUG]
    };

    enqueue(&entry);
}

/*
 * renderqueue_enqueue_brick_path()
 * Enqueues the path of a moving brick (editor)
 */
void renderqueue_enqueue_brick_path(brick_t *brick)
{
    renderqueue_entry_t entry = {
        .renderable.brick = brick,
        .vtable = &VTABLE[TYPE_BRICK_PATH]
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
        .renderable.item = item,
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
        .renderable.object = object,
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
        .renderable.player = player,
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
        .renderable.dummy = NULL,
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
        .renderable.ssobject = object,
        .vtable = &VTABLE[TYPE_SSOBJECT]
    };

    /* skip if the object is not a renderable */
    if(!surgescript_object_has_tag(object, "renderable"))
        return;

    /* don't enqueue invisible renderables */
    if(surgescript_object_has_function(object, "get_visible")) {
        surgescript_var_t* ret = surgescript_var_create();
        surgescript_object_call_function(object, "get_visible", NULL, 0, ret);
        bool is_visible = surgescript_var_get_bool(ret);
        surgescript_var_destroy(ret);

        if(!is_visible)
            return;
    }

    /* enqueue */
    enqueue(&entry);
}

/*
 * renderqueue_enqueue_ssobject_debug()
 * Enqueues a SurgeScript object (editor)
 */
void renderqueue_enqueue_ssobject_debug(surgescript_object_t* object)
{
    renderqueue_entry_t entry = {
        .renderable.ssobject = object,
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
        .renderable.ssobject = object,
        .vtable = &VTABLE[TYPE_SSOBJECT_GIZMO]
    };

    /* skip if the object is not a gizmo */
    if(!surgescript_object_has_tag(object, "gizmo"))
        return;

    /* enqueue */
    enqueue(&entry);
}

/*
 * renderqueue_enqueue_background()
 * Enqueues the background
 */
void renderqueue_enqueue_background(bgtheme_t* background)
{
    renderqueue_entry_t entry = {
        .renderable.theme = background,
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
        .renderable.theme = foreground,
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
        .renderable.dummy = NULL,
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
    float za = a->vtable->zindex(a->renderable);
    float zb = b->vtable->zindex(b->renderable);

    if(fabs(za - zb) * 10.0f < ZINDEX_OFFSET(1)) {
        int ta = a->vtable->type(a->renderable);
        int tb = b->vtable->type(b->renderable);

        if(ta == tb)
            return a->vtable->ypos(a->renderable) - b->vtable->ypos(b->renderable);
        else
            return (ta == TYPE_PLAYER) - (tb == TYPE_PLAYER); /* render the players in front of the other entries if all else is equal */
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

/* generates a random string */
const char* random_path(char prefix)
{
    static char buffer[8], table[] = {
        [0x0] = '0', [0x1] = '1', [0x2] = '2', [0x3] = '3',
        [0x4] = '4', [0x5] = '5', [0x6] = '6', [0x7] = '7',
        [0x8] = '8', [0x9] = '9', [0xa] = 'a', [0xb] = 'b',
        [0xc] = 'c', [0xd] = 'd', [0xe] = 'e', [0xf] = 'f'
    };

    int x = random(65536);

    buffer[0] = '<';
    buffer[1] = prefix;
    buffer[2] = table[(x >> 12) & 0xf];
    buffer[3] = table[(x >> 8) & 0xf];
    buffer[4] = table[(x >> 4) & 0xf];
    buffer[5] = table[x & 0xf];
    buffer[6] = '>';
    buffer[7] = '\0';

    return buffer;
}




/* ----- private strategies ----- */

int type_particles(renderable_t r) { return TYPE_PARTICLE; }
int type_player(renderable_t r) { return TYPE_PLAYER; }
int type_item(renderable_t r) { return TYPE_ITEM; }
int type_object(renderable_t r) { return TYPE_OBJECT; }
int type_brick(renderable_t r) { return TYPE_BRICK; }
int type_brick_mask(renderable_t r) { return TYPE_BRICK_MASK; }
int type_brick_debug(renderable_t r) { return TYPE_BRICK_DEBUG; }
int type_brick_path(renderable_t r) { return TYPE_BRICK_PATH; }
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
float zindex_brick_debug(renderable_t r) { return zindex_brick(r); }
float zindex_brick_path(renderable_t r) { return zindex_brick_mask(r) + 1.0f; }
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
int ypos_brick_debug(renderable_t r) { return ypos_brick(r); }
int ypos_brick_path(renderable_t r) { return ypos_brick(r); }
int ypos_ssobject(renderable_t r) { return 0; } /* TODO (not needed?) */
int ypos_ssobject_debug(renderable_t r) { return ypos_ssobject(r); }
int ypos_ssobject_gizmo(renderable_t r) { return ypos_ssobject(r); }
int ypos_background(renderable_t r) { return 0; } /* preserve relative indexes */
int ypos_foreground(renderable_t r) { return 0; } /* preserve relative indexes */
int ypos_water(renderable_t r) { return 0; } /* not needed */

const char* path_particles(renderable_t r, char* dest, size_t dest_size) { return str_cpy(dest, "<particles>", dest_size); }
const char* path_player(renderable_t r, char* dest, size_t dest_size) { return str_cpy(dest, image_filepath(actor_image(r.player->actor)), dest_size); }
const char* path_item(renderable_t r, char* dest, size_t dest_size) { return str_cpy(dest, "<legacy-item>", dest_size); }
const char* path_object(renderable_t r, char* dest, size_t dest_size) { return str_cpy(dest, "<legacy-object>", dest_size); }
const char* path_brick(renderable_t r, char* dest, size_t dest_size) { return str_cpy(dest, image_filepath(brick_image(r.brick)), dest_size); }
const char* path_brick_mask(renderable_t r, char* dest, size_t dest_size) { return str_cpy(dest, random_path('M'), dest_size); }
const char* path_brick_debug(renderable_t r, char* dest, size_t dest_size) { return path_brick(r, dest, dest_size); }
const char* path_brick_path(renderable_t r, char* dest, size_t dest_size) { return str_cpy(dest, random_path('P'), dest_size); }
const char* path_background(renderable_t r, char* dest, size_t dest_size) { return str_cpy(dest, "<background>", dest_size); }
const char* path_foreground(renderable_t r, char* dest, size_t dest_size) { return str_cpy(dest, "<foreground>", dest_size); }
const char* path_water(renderable_t r, char* dest, size_t dest_size) { return str_cpy(dest, "<water>", dest_size); }

const char* path_ssobject(renderable_t r, char* dest, size_t dest_size)
{
    if(surgescript_object_has_function(r.ssobject, "get_filepathOfRenderable")) {
        surgescript_var_t* ret = surgescript_var_create();

        surgescript_object_call_function(r.ssobject, "get_filepathOfRenderable", NULL, 0, ret);
        str_cpy(dest, surgescript_var_fast_get_string(ret), dest_size);

        surgescript_var_destroy(ret);
        return dest;
    }

    /*str_cpy(dest, surgescript_object_name(r.ssobject), dest_size);*/
    return str_cpy(dest, random_path('S'), dest_size);
}

const char* path_ssobject_debug(renderable_t r, char* dest, size_t dest_size)
{
    /* this routine is based on render_ssobject_debug() */
    const char* name = surgescript_object_name(r.ssobject);
    const animation_t* anim = sprite_animation_exists(name, 0) ? sprite_get_animation(name, 0) : sprite_get_animation(NULL, 0);
    const image_t* img = sprite_get_image(anim, 0);

    return str_cpy(dest, image_filepath(img), dest_size);
}

const char* path_ssobject_gizmo(renderable_t r, char* dest, size_t dest_size)
{
    return str_cpy(dest, random_path('G'), dest_size);
}




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

void render_brick_debug(renderable_t r, v2d_t camera_position)
{
    brick_render_debug(r.brick, camera_position);
}

void render_brick_path(renderable_t r, v2d_t camera_position)
{
    brick_render_path(r.brick, camera_position);
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
