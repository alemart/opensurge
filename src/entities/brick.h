/*
 * Open Surge Engine
 * brick.h - brick module
 * Copyright (C) 2008-2010, 2012, 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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
#include "../core/image.h"

/* brick type */
enum brickproperty_t
{
    BRK_NONE,           /* passable brick */
    BRK_OBSTACLE,       /* solid bricks (floors, walls, etc) */
    BRK_CLOUD           /* clouds (one-way platforms) */
};

/* brick behavior */
enum brickbehavior_t {
    BRB_DEFAULT,        /* static bricks */
    BRB_CIRCULAR,       /* bricks with elliptical movement */
    BRB_BREAKABLE,      /* bricks that can be broken */
    BRB_FALL            /* bricks that are broken once you step on them */
};

/* brick layer (loop system) */
enum bricklayer_t {
    BRL_DEFAULT,
    BRL_GREEN,
    BRL_YELLOW
};

/* forward declarations */
struct actor_t;
struct player_t;
struct item_list_t;
struct enemy_list_t;
struct collisionmask_t;
struct obstacle_t;
struct brickdata_t;
struct brick_t;

/* typedefs */
typedef struct brick_t brick_t;
typedef struct brick_list_t brick_list_t;
typedef enum brickproperty_t brickproperty_t;
typedef enum brickbehavior_t brickbehavior_t;
typedef enum bricklayer_t bricklayer_t;

/* linked list of bricks */
struct brick_list_t {
    brick_t *data;
    brick_list_t *next;
};

/* brickset: theme interface */
void brickset_load(const char *filename); /* loads a brickset */
void brickset_unload(); /* unloads the current brickset */
int brickset_size(); /* number of bricks */
int brickset_loaded(); /* is a brickset loaded? */

/* brick interface */
brick_t* brick_create(int id, v2d_t position, bricklayer_t layer); /* spawns a new brick at the specified position */
brick_t* brick_destroy(brick_t *brk); /* destroys an existing brick */
void brick_update(brick_t *brk, struct player_t** team, int team_size, struct brick_list_t *brick_list, struct item_list_t *item_list, struct enemy_list_t *enemy_list); /* updates a brick */
void brick_render(brick_t *brk, v2d_t camera_position); /* renders a brick */

/* brick properties & operations */
int brick_id(const brick_t* brk); /* brick id (its number in the brickset) */
brickproperty_t brick_type(const brick_t* brk); /* brick type */
brickbehavior_t brick_behavior(const brick_t* brk); /* brick behavior */
bricklayer_t brick_layer(const brick_t* brk); /* brick layer */
const image_t* brick_image(const brick_t *brk); /* returns the image of the brick */
const struct collisionmask_t* brick_collisionmask(const brick_t *brk); /* returns the collision mask of the brick (may be NULL) */
const struct obstacle_t* brick_obstacle(const brick_t* brk); /* returns the obstacle associated with this brick (may be NULL) */
float brick_zindex(const brick_t* brk); /* brick zindex */
v2d_t brick_position(const brick_t* brk); /* brick position */
v2d_t brick_spawnpoint(const brick_t* brk); /* brick spawn point */
v2d_t brick_size(const brick_t* brk); /* brick size, in pixels */
void brick_kill(brick_t* brk); /* kills a brick */
int brick_is_alive(const brick_t* brk); /* checks if a brick is alive */

/* brick utilities */
uint32 bricklayer2color(bricklayer_t layer);
const char* bricklayer2colorname(bricklayer_t layer);
bricklayer_t colorname2bricklayer(const char *name);
v2d_t brick_movable_platform_offset(const brick_t *brk); /* movable platforms must move actors on top of them. Returns a delta_space vector */
void brick_render_path(const brick_t *brk, v2d_t camera_position); /* movable platforms path */
const char* brick_get_property_name(brickproperty_t property); /* property name */
const char* brick_get_behavior_name(brickbehavior_t behavior); /* behavior name */
int brick_exists(int id); /* does a brick with the given id exist? */
const image_t* brick_image_preview(int id); /* image of the brick with the given id (may be NULL) */

#endif
