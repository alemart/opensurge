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

/* The SurgeEngine object
   coded in SurgeScript */
static const char* code = ""
    "@Plugin"                                                               "\n"
    "object \"SurgeEngine\""                                                "\n"
    "{"                                                                     "\n"
        "actorFactory = spawn(\"ActorFactory\");"                           "\n"
        ""                                                                  "\n"
        "fun get_Actor()"                                                   "\n"
        "{"                                                                 "\n"
            "return actorFactory;"                                          "\n"
        "}"                                                                 "\n"
    "}"                                                                     "\n"
    ""                                                                      "\n"
    ""                                                                      "\n"
    ""                                                                      "\n"
    "object \"ActorFactory\""                                               "\n"
    "{"                                                                     "\n"
        "fun call(spriteName)"                                              "\n"
        "{"                                                                 "\n"
            "actor = caller.spawn(\"Actor\");"                              "\n"
            "actor.__sprite = spriteName;"                                  "\n"
            "return actor;"                                                 "\n"
        "}"                                                                 "\n"
    "}"                                                                     "\n"
;

/*
 * scripting_register_surgeengine()
 * Register SurgeEngine
 */
void scripting_register_surgeengine(surgescript_vm_t* vm)
{
    surgescript_vm_compile_code_in_memory(vm, code);
}