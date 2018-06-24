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

/* The SurgeEngine object
   coded in SurgeScript */
static const char* code = ""
    "@Plugin"                                                               "\n"
    "object \"SurgeEngine\""                                                "\n"
    "{"                                                                     "\n"
        "actorFactory = spawn(\"ActorFactory\");"                           "\n"
        "sensorFactory = spawn(\"SensorFactory\");"                         "\n"
        "camera = spawn(\"Camera\");"                                       "\n"
        "mouse = spawn(\"Mouse\");"                                         "\n"
        ""                                                                  "\n"
        "fun get_Actor()"                                                   "\n"
        "{"                                                                 "\n"
            "return actorFactory;"                                          "\n"
        "}"                                                                 "\n"
        ""                                                                  "\n"
        "fun get_Sensor()"                                                  "\n"
        "{"                                                                 "\n"
            "return sensorFactory;"                                         "\n"
        "}"                                                                 "\n"
        ""                                                                  "\n"
        "fun get_Camera()"                                                  "\n"
        "{"                                                                 "\n"
            "return camera;"                                                "\n"
        "}"                                                                 "\n"
        ""                                                                  "\n"
        "fun get_Mouse()"                                                   "\n"
        "{"                                                                 "\n"
            "return mouse;"                                                 "\n"
        "}"                                                                 "\n"
        ""                                                                  "\n"
        "fun destroy() {}"                                                  "\n"
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
    ""                                                                      "\n"
    ""                                                                      "\n"
    ""                                                                      "\n"
    "object \"SensorFactory\""                                              "\n"
    "{"                                                                     "\n"
        "obstacleMap = spawn(\"ObstacleMap\");"                             "\n"
        ""                                                                  "\n"
        "fun call(x, y, w, h)"                                              "\n"
        "{"                                                                 "\n"
            "if(Math.min(w, h) == 1) {"                                     "\n"
                "sensor = caller.spawn(\"Sensor\");"                        "\n"
                "sensor.__init(x, y, x + w - 1, y + h - 1, obstacleMap);"   "\n"
                "return sensor;"                                            "\n"
            "}"                                                             "\n"
            "else {"                                                        "\n"
                "message = \"Invalid sensor dimensions for \";"             "\n"
                "Application.crash(message + caller.__name);"               "\n"
                "return null;"                                              "\n"
            "}"                                                             "\n"
        "}"                                                                 "\n"
    "}"                                                                     "\n"
;

/* stuff coded in C */
static surgescript_var_t* fun_getversion(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/*
 * scripting_register_surgeengine()
 * Register SurgeEngine
 */
void scripting_register_surgeengine(surgescript_vm_t* vm)
{
    surgescript_vm_compile_code_in_memory(vm, code);
    surgescript_vm_bind(vm, "SurgeEngine", "get_version", fun_getversion, 0);
}

/* SurgeEngine version */
surgescript_var_t* fun_getversion(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_string(surgescript_var_create(), GAME_VERSION_STRING);
}