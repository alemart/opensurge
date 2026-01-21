/*
 * Open Surge Engine
 * events.c - scripting system: SurgeEngine Events
 * Copyright 2008-2026 Alexandre Martins <alemartf(at)gmail.com>
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
    fun toString() { return '[missing event]'; } \n\
    fun destroy() { } \n\
} \n\
\n\
object 'EntityEvent' is 'event' \n\
{ \n\
    target = ''; \n\
    method = 'call'; \n\
    params = []; \n\
    trgnam = ''; \n\
\n\
    fun __init(entityId) \n\
    { \n\
        if(typeof(entityId) == 'object' && entityId.hasTag('entity')) { \n\
            target = entityId; \n\
            trgnam = target.__name; \n\
        } \n\
        else \n\
            target = String(entityId || ''); \n\
        return this; \n\
    } \n\
\n\
    fun willCall(functionName) \n\
    { \n\
        method = String(functionName || 'call'); \n\
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
        entity = unique(target); " /* select by entity id */ " \n\
        if(entity !== null) { \n\
            if(entity.hasFunction(method)) { \n\
                if(entity.__arity(method) == params.length) \n\
                    entity.__invoke(method, params); \n\
                else \n\
                    Console.print(this.__name + ': incorrect arguments for ' + method); \n\
            } \n\
            else \n\
                Console.print(this.__name + ': undefined function ' + method); \n\
        } \n\
        else if((entity = Level.findEntity(target)) !== null) { " /* select by entity name */ " \n\
            if(entity.hasFunction(method)) { \n\
                if(entity.__arity(method) == params.length) { \n\
                    entities = Level.findEntities(target); \n\
                    length = entities.length; \n\
                    for(i = 0; i < length; i++) { \n\
                        entity = entities[i]; \n\
                        entity.__invoke(method, params); \n\
                    } \n\
                } \n\
                else \n\
                    Console.print(this.__name + ': incorrect arguments for ' + method); \n\
            } \n\
            else \n\
                Console.print(this.__name + ': undefined function ' + method); \n\
        } \n\
        else \n\
            Console.print(this.__name + ': missing entity ' + target); \n\
    } \n\
\n\
    fun unique(target) \n\
    { \n\
        return (target != null && typeof(target) == 'object' && target.__name == trgnam) ? target : Level.entity(target);\n\
    } \n\
\n\
    fun toString() \n\
    { \n\
        entity = Level.entity(target); \n\
        if(entity !== null) \n\
            return 'EntityEvent[' + (entity.__name + '.' + method) + ']'; \n\
        else \n\
            return 'EntityEvent[missing link]'; \n\
    } \n\
\n\
    fun destroy() { } \n\
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
            if(functor.__arity(method) == params.length) { \n\
                if(functor.__name === target)" /* just to be sure */ " \n\
                    functor.__invoke(method, params); \n\
            } \n\
            else \n\
                Console.print(this.__name + ': incorrect arguments for ' + target); \n\
        } \n\
        else \n\
            Console.print(this.__name + ': undefined function object ' + target); \n\
    } \n\
\n\
    fun toString() \n\
    { \n\
        return 'FunctionEvent[' + target + ']'; \n\
    } \n\
\n\
    fun destroy() { } \n\
} \n\
\n\
object 'DelayedEvent' is 'event' \n\
{ \n\
    event = null; \n\
    timer = 0; \n\
    delay = 0; \n\
\n\
    state 'main' \n\
    { \n\
    } \n\
\n\
    state 'active' \n\
    { \n\
        timer += Time.delta; \n\
        if(timer >= delay) { \n\
            if(event != null) \n\
                event.call(); \n\
            state = 'main'; \n\
        } \n\
    } \n\
\n\
    fun __init(theEvent) \n\
    { \n\
        if(theEvent.hasTag('event')) \n\
            event = theEvent; \n\
        return this; \n\
    } \n\
\n\
    fun willWait(seconds) \n\
    { \n\
        delay = Math.max(seconds, 0); \n\
        return this; \n\
    } \n\
\n\
    fun call() \n\
    { \n\
        timer = 0; \n\
        state = 'active'; \n\
    } \n\
\n\
    fun toString() \n\
    { \n\
        return 'DelayedEvent[' + delay + ']'; \n\
    } \n\
\n\
    fun destroy() { } \n\
} \n\
\n\
object 'EventList' is 'event' \n\
{ \n\
    events = []; \n\
\n\
    fun __init(list) \n\
    { \n\
        if(typeof(list) == 'object' && list.__name == 'Array') { \n\
            for(j = 0; j < list.length; j++) { \n\
                event = list[j]; \n\
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
\n\
    fun toString() \n\
    { \n\
        return 'EventList[' + events.length + ']'; \n\
    } \n\
\n\
    fun destroy() { } \n\
} \n\
\n\
object 'EventChain' is 'event' \n\
{ \n\
    events = []; \n\
    index = 0; \n\
    loop = false; \n\
\n\
    state 'main' \n\
    { \n\
    } \n\
\n\
    fun __init(list) \n\
    { \n\
        if(typeof(list) == 'object' && list.__name == 'Array') { \n\
            for(j = 0; j < list.length; j++) { \n\
                event = list[j]; \n\
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
        if(events.length > 0) { \n\
            events[index].call(); \n\
            if(++index >= events.length) \n\
                index = loop ? 0 : index - 1; \n\
        } \n\
    } \n\
\n\
    fun willLoop() \n\
    { \n\
        loop = true; \n\
        return this; \n\
    } \n\
\n\
    fun toString() \n\
    { \n\
        return 'EventChain[' + events.length + ']'; \n\
    } \n\
\n\
    fun destroy() { } \n\
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