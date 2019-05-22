/*
 * Open Surge Engine
 * time.c - scripting system: time object
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
#include "../core/timer.h"

/* private */
static surgescript_var_t* fun_getdelta(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/*
 * scripting_register_time()
 * Register the engine-replacement for Time
 */
void scripting_register_time(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "Time", "get_delta", fun_getdelta, 0);
}

/* Time routines */

/* the time (in seconds) taken to complete the last frame */
surgescript_var_t* fun_getdelta(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* get the engine delta, rather than the SurgeScript delta, for synchronized results */
    return surgescript_var_set_number(surgescript_var_create(), timer_get_delta());
}