/*
 * Open Surge Engine
 * events.c - scripting system: SurgeEngine Events
 * Copyright (C) 2019  Alexandre Martins <alemartf@gmail.com>
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

/* SurgeScript code */
static const char code_in_surgescript[] = "\
using SurgeEngine.Level; \n\
\n\
object 'Event' is 'event' \n\
{ \n\
    fun call() { } \n\
    fun toString() { return '(missing event)'; } \n\
} \n\
\n\
object 'EntityEvent' is 'event' \n\
{ \n\
    target = null; \n\
    method = 'call'; \n\
    params = []; \n\
\n\
    fun __init(entityId) \n\
    { \n\
        target = entityId || ''; \n\
        return this; \n\
    } \n\
\n\
    fun willCall(functionName) \n\
    { \n\
        method = functionName || 'call'; \n\
        return this; \n\
    } \n\
\n\
    fun withArgument(x) \n\
    { \n\
        params.push(x); \n\
        return this; \n\
    } \n\
\n\
    fun call() \n\
    { \n\
        entity = Level.entity(target); \n\
        if(entity !== null && entity.hasFunction(method)) \n\
            entity.__invoke(method, params); \n\
    } \n\
} \n\
\n\
object 'EventList' is 'event' \n\
{ \n\
    events = []; \n\
\n\
    fun __init(eventList) \n\
    { \n\
        if(typeof(eventList) == 'object' && eventList.__name == 'Array') { \n\
            for(j = 0; j < eventList.length; j++) { \n\
                event = eventList[j]; \n\
                if(event.hasTag('event')) \n\
                    events.push(event); \n\
            } \n\
        } \n\
\n\
        return this; \n\
    } \n\
\n\
    fun call() \n\
    { \n\
        for(j = 0; j < events.length; j++) \n\
            events[j].call(); \n\
    } \n\
} \n\
\n\
object 'FunctionEvent' is 'event' \n\
{ \n\
    target = ''; \n\
    method = 'call'; \n\
    functor = null; \n\
    params = []; \n\
\n\
    fun __init(func) \n\
    { \n\
        if(typeof(func) == 'object') { \n\
            functor = func;" /* warning: missing references */ " \n\
            target = functor.__name; \n\
        } \n\
        else \n\
            target = String(func || ''); \n\
\n\
        return this; \n\
    } \n\
\n\
    fun withArgument(x) \n\
    { \n\
        params.push(x); \n\
        return this; \n\
    } \n\
\n\
    fun call() \n\
    { \n\
        if(!functor && target) \n\
            functor = spawn(target); \n\
\n\
        if(functor != null && functor.hasFunction(method)) { \n\
            if(functor.__name === target)" /* just to be sure */ " \n\
                functor.__invoke(method, params); \n\
        } \n\
    } \n\
} \n\
";

/*
 * scripting_register_events()
 * Register event types
 */
void scripting_register_events(surgescript_vm_t* vm)
{
    surgescript_vm_compile_code_in_memory(vm, code_in_surgescript);
}