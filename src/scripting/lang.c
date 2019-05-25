/*
 * Open Surge Engine
 * lang.c - scripting system: language system
 * Copyright (C) 2018  Alexandre Martins <alemartf@gmail.com>
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
#include "../core/lang.h"

/* private */
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_get(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/*
 * scripting_register_lang()
 * Register the Lang object
 */
void scripting_register_lang(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "Lang", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Lang", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "Lang", "spawn", fun_spawn, 1);
    surgescript_vm_bind(vm, "Lang", "get", fun_get, 1);
}

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

/* get */
surgescript_var_t* fun_get(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const char* key = surgescript_var_fast_get_string(param[0]);
    char buf[1024];

    lang_getstring(key, buf, sizeof(buf));

    return surgescript_var_set_string(surgescript_var_create(), buf);
}