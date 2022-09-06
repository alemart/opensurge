/*
 * Open Surge Engine
 * prefs.c - scripting system: prefs
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
#include "../core/prefs.h"
#include "../core/modmanager.h"

/* private */
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_get(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_set(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_has(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_delete(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_save(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_clear(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/*
 * scripting_register_prefs()
 * Register the Prefs object
 */
void scripting_register_prefs(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "Prefs", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Prefs", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "Prefs", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "Prefs", "spawn", fun_spawn, 1);
    surgescript_vm_bind(vm, "Prefs", "get", fun_get, 1);
    surgescript_vm_bind(vm, "Prefs", "set", fun_set, 2);
    surgescript_vm_bind(vm, "Prefs", "has", fun_has, 1);
    surgescript_vm_bind(vm, "Prefs", "delete", fun_delete, 1);
    surgescript_vm_bind(vm, "Prefs", "save", fun_save, 0);
    surgescript_vm_bind(vm, "Prefs", "clear", fun_clear, 0);
}

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_object_set_active(object, false);
    return NULL;
}

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    prefs_t* prefs = modmanager_prefs();
    surgescript_object_set_userdata(object, prefs);
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
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    prefs_t* prefs = (prefs_t*)surgescript_object_userdata(object);
    surgescript_var_t* value = surgescript_var_create();
    char* key = surgescript_var_get_string(param[0], manager);

    switch(prefs_item_type(prefs, key)) {
        case 's':
            surgescript_var_set_string(value, prefs_get_string(prefs, key));
            break;
        case 'i':
            surgescript_var_set_number(value, prefs_get_int(prefs, key));
            break;
        case 'f':
            surgescript_var_set_number(value, prefs_get_double(prefs, key));
            break;
        case 'b':
            surgescript_var_set_bool(value, prefs_get_bool(prefs, key));
            break;
        default:
            surgescript_var_set_null(value);
            break;
    }

    ssfree(key);
    return value;
}

/* set */
surgescript_var_t* fun_set(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    prefs_t* prefs = (prefs_t*)surgescript_object_userdata(object);
    char* key = surgescript_var_get_string(param[0], manager);

    switch(surgescript_var_typecode(param[1])) {
        case 'b':
            prefs_set_bool(prefs, key, surgescript_var_get_bool(param[1]));
            break;
        case 'n':
            prefs_set_double(prefs, key, surgescript_var_get_number(param[1]));
            break;
        case 's':
            prefs_set_string(prefs, key, surgescript_var_fast_get_string(param[1]));
            break;
        case 'o': {
            /* convert object to string: using toString() */
            surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(param[1]);
            surgescript_object_t* obj = surgescript_objectmanager_get(manager, handle);
            if(obj != NULL) {
                surgescript_var_t* tmp = surgescript_var_create();
                char* str = NULL;
                surgescript_object_call_function(obj, "toString", NULL, 0, tmp);
                str = surgescript_var_get_string(tmp, manager);
                prefs_set_string(prefs, key, str);
                ssfree(str);
                surgescript_var_destroy(tmp);
            }
            else
                prefs_set_null(prefs, key);
            break;
        }
        default:
            prefs_set_null(prefs, key);
            break;
    }

    ssfree(key);
    return NULL;
}

/* has */
surgescript_var_t* fun_has(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    prefs_t* prefs = (prefs_t*)surgescript_object_userdata(object);
    surgescript_var_t* value = surgescript_var_create();
    char* key = surgescript_var_get_string(param[0], manager);
    surgescript_var_set_bool(value, prefs_has_item(prefs, key));
    ssfree(key);
    return value;
}

/* delete */
surgescript_var_t* fun_delete(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    prefs_t* prefs = (prefs_t*)surgescript_object_userdata(object);
    char* key = surgescript_var_get_string(param[0], manager);
    prefs_delete_item(prefs, key);
    ssfree(key);
    return NULL;
}

/* save */
surgescript_var_t* fun_save(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    prefs_t* prefs = (prefs_t*)surgescript_object_userdata(object);
    prefs_save(prefs);
    return NULL;
}

/* clear */
surgescript_var_t* fun_clear(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    prefs_t* prefs = (prefs_t*)surgescript_object_userdata(object);
    prefs_clear(prefs);
    return NULL;
}
