/*
 * Open Surge Engine
 * obstaclemap.c - scripting system: the bridge between level obstacles and SurgeScript
 * Copyright (C) 2018, 2019 Alexandre Martins <alemartf@gmail.com>
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
#include "../entities/entitymanager.h"
#include "../physics/obstacle.h"
#include "../physics/obstaclemap.h"
#include "../physics/collisionmask.h"
#include "../scenes/level.h"
#include "../entities/player.h"

/* private */
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
obstaclemap_t* scripting_obstaclemap_ptr(const surgescript_object_t* object);

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


/*
 * scripting_obstaclemap_ptr()
 * Get the obstaclemap_t*
 */
obstaclemap_t* scripting_obstaclemap_ptr(const surgescript_object_t* object)
{
    player_t* player = level_player();
    obstaclemap_t* emptymap = (obstaclemap_t*)surgescript_object_userdata(object);
    return player != NULL ? player->obstaclemap : emptymap;
}

/* private */

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* do nothing */
    return NULL;
}

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* create an empty obstacle map */
    obstaclemap_t* obstaclemap = obstaclemap_create();
    surgescript_object_set_userdata(object, obstaclemap);
    return NULL;
}

/* destructor */
surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* destroy the obstacle map */
    obstaclemap_t* obstaclemap = scripting_obstaclemap_ptr(object);
    obstaclemap_destroy(obstaclemap);
    surgescript_object_set_userdata(object, NULL);
    return NULL;
}