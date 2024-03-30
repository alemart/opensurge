/*
 * Open Surge Engine
 * game.c - scripting system: game settings
 * Copyright 2008-2024 Alexandre Martins <alemartf(at)gmail.com>
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
#include "../core/config.h"

/* private */
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_gettitle(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getversion(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/*
 * scripting_register_game()
 * Register the Game settings object
 */
void scripting_register_game(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "GameSettings", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "GameSettings", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "GameSettings", "spawn", fun_spawn, 1);
    surgescript_vm_bind(vm, "GameSettings", "get_title", fun_gettitle, 0);
    surgescript_vm_bind(vm, "GameSettings", "get_version", fun_getversion, 0);
}

/* Routines of the Game settings object */

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_object_set_active(object, false);
    return NULL;
}

/* destroy */
surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* disabled */
    return NULL;
}

/* spawn */
surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* disabled */
    return NULL;
}

/* the title of the game that is running on the engine. Returns an empty string if undefined. */
surgescript_var_t* fun_gettitle(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const char* game_title = config_game_title("");

    return surgescript_var_set_string(surgescript_var_create(), game_title);
}

/* the version string of the game that is running on the engine. Not to be
   confused with the engine version! Returns an empty string if undefined. */
surgescript_var_t* fun_getversion(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const char* game_version = config_game_version("");

    return surgescript_var_set_string(surgescript_var_create(), game_version);
}