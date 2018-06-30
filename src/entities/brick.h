/*
 * Open Surge Engine
 * brick.h - brick module
 * Copyright (C) 2008-2010, 2012  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#ifndef _BRICK_H
#define _BRICK_H

#include "../core/v2d.h"
#include "../core/sprite.h"

/* brick properties */
enum brickproperty_t
{
    BRK_NONE,           /* passable brick */
    BRK_OBSTACLE,       /* non-passable bricks (walls, etc) */
    BRK_CLOUD           /* clouds (one-way platforms) */
};

/* brick behavior */
enum brickbehavior_t {
    BRB_DEFAULT,        /* static bricks */
    BRB_CIRCULAR,       /* bricks with elliptical movement */
    BRB_BREAKABLE,      /* bricks that can be broken */
    BRB_FALL            /* bricks that are broken once you step on them */
};

/* brick state */
enum brickstate_t {
    BRS_IDLE,           /* the brick is alive, idle */
    BRS_DEAD,           /* must be removed from the level */
    BRS_ACTIVE,         /* generic action */
};

/* brick layer (loop system) */
enum bricklayer_t {
    BRL_DEFAULT,
    BRL_GREEN,
    BRL_YELLOW
};

/* misc */
#define BRICK_MAXVALUES         2
#define BRICKBEHAVIOR_MAXARGS   5



/* forward declarations */
struct actor_t;
struct player_t;
struct item_list_t;
struct enemy_list_t;
struct collisionmask_t;
typedef struct brickdata_t brickdata_t;
typedef struct brick_t brick_t;
typedef struct brick_list_t brick_list_t;
typedef enum brickstate_t brickstate_t;
typedef enum brickbehavior_t brickbehavior_t;
typedef enum brickproperty_t brickproperty_t;
typedef enum bricklayer_t bricklayer_t;

/* brick theme: meta data of bricks */
struct brickdata_t {
    spriteinfo_t *data; /* this is not stored in the main hash */
    image_t *image; /* pointer to the current brick image in the animation */
    struct collisionmask_t *mask; /* collision mask */
    int angle; /* in degrees, 0 <= angle < 360 */
    float zindex; /* 0.0 (background) <= z-index <= 1.0 (foreground) */
    brickproperty_t property;
    brickbehavior_t behavior;
    float behavior_arg[BRICKBEHAVIOR_MAXARGS];
};

/* brick instances */
struct brick_t { /* a real, concrete brick */
    brickdata_t *brick_ref; /* brick metadata */
    int x, y; /* current position */
    int sx, sy; /* spawn point */
    int enabled; /* obsolete: old loop system */
    brickstate_t state; /* brick state: BRS_* */
    float value[BRICK_MAXVALUES]; /* alterable values */
    float animation_frame; /* controlled by a timer */
    bricklayer_t layer; /* loop system: BRL_* */
};

/* linked list of bricks */
struct brick_list_t {
    brick_t *data;
    brick_list_t *next;
};

/* brick theme interface */
void brickdata_load(const char *filename); /* loads a brick theme */
void brickdata_unload(); /* unloads the current brick theme */
brickdata_t *brickdata_get(int id); /* returns the specified brickdata */
int brickdata_size(); /* number of bricks */

/* brick interface */
brick_t* brick_create(int id); /* spawns a new brick */
brick_t* brick_destroy(brick_t *brk); /* destroys an existing brick */
void brick_update(brick_t *brk, struct player_t** team, int team_size, struct brick_list_t *brick_list, struct item_list_t *item_list, struct enemy_list_t *enemy_list); /* updates a brick */
void brick_render(brick_t *brk, v2d_t camera_position); /* renders a brick */

v2d_t brick_moveable_platform_offset(const brick_t *brk); /* moveable platforms must move actors on top of them. Returns a delta_space vector */
void brick_render_path(const brick_t *brk, v2d_t camera_position); /* moveable platforms path */
const char* brick_get_property_name(brickproperty_t property); /* property name */
const char* brick_get_behavior_name(brickbehavior_t behavior); /* behavior name */
const image_t* brick_image(const brick_t *brk); /* returns the image of the brick */
const struct collisionmask_t* brick_collisionmask(const brick_t *brk); /* returns the collision mask (will never be null) */

/* brick utilities */
uint32 bricklayer2color(bricklayer_t layer);
const char* bricklayer2colorname(bricklayer_t layer);
bricklayer_t colorname2bricklayer(const char *name);

#endif
