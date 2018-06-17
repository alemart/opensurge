/*
 * Open Surge Engine
 * obstaclemap.c - scripting system: the bridge between level obstacles and SurgeScript
 * Copyright (C) 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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
#include <stdbool.h>
#include "../core/v2d.h"
#include "../core/image.h"
#include "../entities/brick.h"
#include "../entities/collisionmask.h"
#include "../entities/entitymanager.h"
#include "../entities/physics/obstacle.h"
#include "../entities/physics/obstaclemap.h"

/* private */
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static obstaclemap_t* create_obstaclemap();
static inline obstacle_t* brick2obstacle(const brick_t *brick);

/*
 * scripting_register_obstaclemap()
 * Register this component
 */
void scripting_register_obstaclemap(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "ObstacleMap", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "ObstacleMap", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "ObstacleMap", "destructor", fun_destructor, 0);
}



/* private */

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    fun_destructor(object, NULL, 0);
    fun_constructor(object, NULL, 0);
    return NULL;
}

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    obstaclemap_t* obstaclemap = create_obstaclemap();
    surgescript_object_set_userdata(object, obstaclemap);
    return NULL;
}

/* destructor */
surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    obstaclemap_t* obstaclemap = (obstaclemap_t*)surgescript_object_userdata(object);
    if(obstaclemap != NULL)
        obstaclemap_destroy(obstaclemap);
    surgescript_object_set_userdata(object, NULL);
    return NULL;
}

/* create a new obstacle map */
obstaclemap_t* create_obstaclemap()
{
    obstaclemap_t* obstaclemap = obstaclemap_create();
    brick_list_t* brick_list = entitymanager_retrieve_active_bricks();
    for(brick_list_t* brick = brick_list; brick != NULL; brick = brick->next) {
        if(brick->data->brick_ref->property != BRK_NONE && brick->data->enabled && brick_image(brick->data) != NULL)
            obstaclemap_add_obstacle(obstaclemap, brick2obstacle(brick->data));
    }
    entitymanager_release_retrieved_brick_list(brick_list);
    return obstaclemap;
}

/* converts a brick to an obstacle */
obstacle_t* brick2obstacle(const brick_t *brick)
{
    const image_t *image = collisionmask_image(brick_collisionmask(brick));
    /*int angle = (int)(256 - (brick->brick_ref->angle % 360) / 1.40625f) % 256;*/
    int angle = brick->brick_ref->angle % 360;
    v2d_t position = v2d_new(brick->x, brick->y);
    bool cloud = (brick->brick_ref->property == BRK_CLOUD);
    return (cloud ? obstacle_create_oneway : obstacle_create_solid)(image, angle, position);
}