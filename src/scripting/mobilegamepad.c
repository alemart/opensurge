/*
 * Open Surge Engine
 * mobilegamepad.c - scripting system: virtual gamepad for mobile devices
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
#include "../core/mobile_gamepad.h"

/* private */
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_fadein(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_fadeout(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/*
 * scripting_register_mobilegamepad()
 * Register the Mobile Gameapd object
 */
void scripting_register_mobilegamepad(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "MobileGamepad", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Lang", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "Lang", "spawn", fun_spawn, 1);
    surgescript_vm_bind(vm, "MobileGamepad", "get_visible", fun_getvisible, 0);
    surgescript_vm_bind(vm, "MobileGamepad", "fadeIn", fun_fadein, 0);
    surgescript_vm_bind(vm, "MobileGamepad", "fadeOut", fun_fadeout, 0);
}

/* Mobile Gamepad routines */

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

/* whether or not the mobile gamepad is visible */
surgescript_var_t* fun_getvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_bool(surgescript_var_create(), mobilegamepad_is_visible());
}

/* make the mobile gamepad visible with a fade effect */
surgescript_var_t* fun_fadein(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    mobilegamepad_fadein();

    return NULL;
}

/* make the mobile gamepad invisible with a fade effect */
surgescript_var_t* fun_fadeout(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    mobilegamepad_fadeout();

    return NULL;
}