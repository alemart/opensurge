/*
 * Open Surge Engine
 * surgeengine.c - scripting system: SurgeEngine object
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
#include "../core/global.h"

/* stuff coded in C */
static surgescript_var_t* fun_getversion(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/*
 * scripting_register_surgeengine()
 * Register SurgeEngine
 */
void scripting_register_surgeengine(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "SurgeEngine", "get_version", fun_getversion, 0);
    surgescript_vm_bind(vm, "SurgeEngine", "destroy", fun_destroy, 0);
}

/* can't destroy this object */
surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return NULL;
}

/* SurgeEngine version */
surgescript_var_t* fun_getversion(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_string(surgescript_var_create(), GAME_VERSION_STRING);
}