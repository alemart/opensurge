/*
 * Open Surge Engine
 * object.c - scripting system: base object
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

#include <string.h>
#include <surgescript.h>
#include "../core/video.h"
#include "../core/logfile.h"

/* private */
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/*
 * scripting_register_object()
 * Register the engine-replacement functions for Object
 */
void scripting_register_object(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "Object", "spawn", fun_spawn, 1);
}

/* Object routines */

/* spawn a child object */
surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* spawn the child */
    const char* child_name = surgescript_var_fast_get_string(param[0]);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t me = surgescript_object_handle(object);
    surgescript_objecthandle_t child_handle = surgescript_objectmanager_spawn(manager, me, child_name, NULL);

    /* check for entity policy violation
       entities must be children of other entities or of Mother Level ;) */
    const surgescript_object_t* child = surgescript_objectmanager_get(manager, child_handle);
    if(surgescript_object_has_tag(child, "entity") && !surgescript_object_has_tag(object, "entity")) {
        const char* parent_name = surgescript_object_name(object);

        if(0 != strcmp(parent_name, "Level")) {
            logfile_message("\"%s\" violates entity policy when spawned by non-entity \"%s\"", child_name, parent_name);
            video_showmessage("\"%s\" violates entity policy when spawned by non-entity \"%s\"", child_name, parent_name);
        }
    }

    /* done! */
    return surgescript_var_set_objecthandle(surgescript_var_create(), child_handle);
}