/*
 * Open Surge Engine
 * surgeengine.c - scripting system: SurgeEngine plugin
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

#include <surgescript.h>
#include "../core/global.h"
#include "../entities/mobilegamepad.h"

/* private */
static surgescript_var_t* fun_getversion(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getmobile(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static const char code_in_surgescript[];

/*
 * scripting_register_surgeengine()
 * Register SurgeEngine
 */
void scripting_register_surgeengine(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "SurgeEngine", "get_version", fun_getversion, 0);
    surgescript_vm_bind(vm, "SurgeEngine", "get_mobile", fun_getmobile, 0);

    surgescript_vm_compile_code_in_memory(vm, code_in_surgescript);
}

/* the version of the game engine, as a string */
surgescript_var_t* fun_getversion(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_string(surgescript_var_create(), GAME_VERSION_STRING);
}

/* checks whether or not the engine has been **SUCCESSFULLY** launched in mobile mode */
surgescript_var_t* fun_getmobile(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_bool(surgescript_var_create(), mobilegamepad_is_available());
}

/* SurgeScript code */
static const char code_in_surgescript[] = "\
@Package \n\
object 'SurgeEngine' \n\
{ \n\
    public readonly Vector2 = spawn('VectorFactory'); \n\
    public readonly Transform = spawn('TransformFactory'); \n\
    public readonly Audio = spawn('Audio'); \n\
    public readonly Video = spawn('Video'); \n\
    public readonly Prefs = spawn('Prefs'); \n\
    public readonly Lang = spawn('Lang'); \n\
    public readonly Web = spawn('Web'); \n\
    public readonly LevelManager = spawn('LevelManager'); \n\
    public readonly Actor = spawn('ActorFactory'); \n\
    public readonly Behaviors = spawn('BehaviorFactory'); \n\
    public readonly Brick = spawn('BrickFactory'); \n\
    public readonly Input = spawn('InputFactory'); \n\
    public readonly Camera = spawn('Camera'); \n\
    public readonly Collisions = spawn('Collision'); \n\
    public readonly Events = spawn('Events'); \n\
    public readonly UI = spawn('UI'); \n\
    public readonly Platform = spawn('Platform'); \n\
    public readonly Game = spawn('GameSettings'); \n\
\n\
    fun get_Level() \n\
    { \n\
        return LevelManager.currentLevel; \n\
    } \n\
\n\
    fun get_Player() \n\
    { \n\
        return LevelManager.currentLevel.__playerManager; \n\
    } \n\
\n\
    fun get_Behavior() { return Behaviors; } \n\
    fun destroy() { } \n\
} \n\
\n\
\n\
\n\
object 'ActorFactory' \n\
{ \n\
    fun call(spriteName) \n\
    { \n\
        actor = caller.spawn('Actor'); \n\
        actor.__init(spriteName); \n\
        return actor; \n\
    } \n\
\n\
    fun destroy() { } \n\
} \n\
\n\
object 'BehaviorFactory' \n\
{ \n\
    public readonly CircularMovement = spawn('CircularMovementFactory'); \n\
    public readonly DirectionalMovement = spawn('DirectionalMovementFactory'); \n\
    public readonly Platformer = spawn('PlatformerFactory'); \n\
    public readonly Enemy = spawn('EnemyFactory'); \n\
\n\
    fun destroy() { } \n\
} \n\
\n\
object 'CircularMovementFactory' { \n\
    fun call() { return caller.spawn('CircularMovement'); } \n\
    fun destroy() { } \n\
} \n\
\n\
object 'DirectionalMovementFactory' { \n\
    fun call() { return caller.spawn('DirectionalMovement'); } \n\
    fun destroy() { } \n\
} \n\
\n\
object 'PlatformerFactory' { \n\
    fun call() { return caller.spawn('Platformer'); } \n\
    fun destroy() { } \n\
} \n\
\n\
object 'EnemyFactory' { \n\
    fun call() { return caller.spawn('Enemy'); } \n\
    fun destroy() { } \n\
} \n\
\n\
object 'BrickFactory' \n\
{ \n\
    fun call(spriteName) \n\
    { \n\
        brick = caller.spawn('Brick'); \n\
        brick.__init(spriteName); \n\
        return brick; \n\
    } \n\
\n\
    fun destroy() { } \n\
} \n\
\n\
object 'SensorFactory' \n\
{ \n\
    obstacleMap = spawn('ObstacleMap'); \n\
\n\
    fun call(x, y, w, h) \n\
    { \n\
        if(Math.min(w, h) == 1) { \n\
            sensor = caller.spawn('Sensor'); \n\
            sensor.__init(x, y, x + w - 1, y + h - 1, obstacleMap); \n\
            return sensor; \n\
        } \n\
        else { \n\
            message = 'Invalid sensor dimensions for '; \n\
            Application.crash(message + caller.__name); \n\
            return null; \n\
        } \n\
    } \n\
\n\
    fun destroy() { } \n\
} \n\
\n\
object 'InputFactory' \n\
{ \n\
    public readonly Mouse = spawn('Mouse'); \n\
    public readonly MobileGamepad = spawn('MobileGamepad'); \n\
\n\
    fun call(inputMap) \n\
    { \n\
        input = caller.spawn('Input'); \n\
        input.__init(inputMap); \n\
        return input; \n\
    } \n\
\n\
    fun destroy() { } \n\
} \n\
\n\
object 'Collision' \n\
{ \n\
    manager = spawn('CollisionManager'); \n\
    public readonly CollisionBox = spawn('CollisionBoxFactory'); \n\
    public readonly CollisionBall = spawn('CollisionBallFactory'); \n\
    public readonly Sensor = spawn('SensorFactory'); \n\
\n\
    fun destroy() { } \n\
} \n\
\n\
object 'CollisionBoxFactory' \n\
{ \n\
    manager = parent.child('CollisionManager'); \n\
\n\
    fun call(width, height) \n\
    { \n\
        return __spawn(caller, width, height); \n\
    } \n\
\n\
    fun __spawn(parnt, width, height) // called by Player \n\
    { \n\
        collider = parnt.spawn('CollisionBox'); \n\
        collider.__init(manager, width, height); \n\
        return collider; \n\
    } \n\
\n\
    fun destroy() { } \n\
} \n\
\n\
object 'CollisionBallFactory' \n\
{ \n\
    manager = parent.child('CollisionManager'); \n\
\n\
    fun call(radius) \n\
    { \n\
        collider = caller.spawn('CollisionBall'); \n\
        collider.__init(manager, radius); \n\
        return collider; \n\
    } \n\
\n\
    fun destroy() { } \n\
} \n\
\n\
object 'UI' \n\
{ \n\
    public readonly Text = spawn('TextFactory'); \n\
\n\
    fun destroy() { } \n\
} \n\
\n\
object 'TextFactory' \n\
{ \n\
    fun call(fontName) \n\
    { \n\
        text = caller.spawn('Text'); \n\
        text.__init(fontName); \n\
        return text; \n\
    } \n\
\n\
    fun destroy() { } \n\
} \n\
\n\
object 'TransformFactory' \n\
{ \n\
    fun call() \n\
    { \n\
        t2 = caller.child('Transform'); \n\
        return t2 != null ? t2 : caller.spawn('Transform'); \n\
    } \n\
     \n\
    fun destroy() { } \n\
} \n\
\n\
object 'VectorFactory' \n\
{ \n\
    public readonly up = spawn('Vector2').__init(0, -1); \n\
    public readonly right = spawn('Vector2').__init(1, 0); \n\
    public readonly down = spawn('Vector2').__init(0, 1); \n\
    public readonly left = spawn('Vector2').__init(-1, 0); \n\
    public readonly zero = spawn('Vector2').__init(0, 0); \n\
    public readonly one = spawn('Vector2').__init(1, 1); \n\
    temp = System.child('__Temp'); \n\
\n\
    fun call(x, y) \n\
    { \n\
        return temp.spawn('Vector2').__init(x, y); \n\
    } \n\
\n\
    fun destroy() { } \n\
} \n\
\n\
object 'Audio' \n\
{ \n\
    public readonly Music = spawn('MusicFactory'); \n\
    public readonly Sound = spawn('SoundFactory'); \n\
\n\
    fun destroy() { } \n\
} \n\
\n\
object 'MusicFactory' \n\
{ \n\
    fun call(pathToMusic) \n\
    { \n\
        return __spawn(caller, pathToMusic); \n\
    } \n\
\n\
    fun __spawn(parnt, pathToMusic) // called by Level \n\
    { \n\
        music = parnt.spawn('Music'); \n\
        music.__init(pathToMusic); \n\
        return music; \n\
    } \n\
\n\
    fun destroy() { } \n\
} \n\
\n\
object 'SoundFactory' \n\
{ \n\
    fun call(pathToSound) \n\
    { \n\
        sound = caller.spawn('Sound'); \n\
        sound.__init(pathToSound); \n\
        return sound; \n\
    } \n\
\n\
    fun destroy() { } \n\
} \n\
\n\
object 'Events' \n\
{ \n\
    public readonly Event = spawn('EventFactory'); \n\
    public readonly EntityEvent = spawn('EntityEventFactory'); \n\
    public readonly FunctionEvent = spawn('FunctionEventFactory'); \n\
    public readonly DelayedEvent = spawn('DelayedEventFactory'); \n\
    public readonly EventList = spawn('EventListFactory'); \n\
    public readonly EventChain = spawn('EventChainFactory'); \n\
\n\
    fun destroy() { } \n\
} \n\
\n\
object 'EventFactory' \n\
{ \n\
    fun call() \n\
    { \n\
        return caller.spawn('Event'); \n\
    } \n\
\n\
    fun destroy() { } \n\
} \n\
\n\
object 'EntityEventFactory' \n\
{ \n\
    fun call(entityId) \n\
    { \n\
        return caller.spawn('EntityEvent').__init(entityId); \n\
    } \n\
\n\
    fun destroy() { } \n\
} \n\
\n\
object 'FunctionEventFactory' \n\
{ \n\
    fun call(objectName) \n\
    { \n\
        return caller.spawn('FunctionEvent').__init(objectName); \n\
    } \n\
\n\
    fun destroy() { } \n\
} \n\
\n\
object 'DelayedEventFactory' \n\
{ \n\
    fun call(event) \n\
    { \n\
        return caller.spawn('DelayedEvent').__init(event); \n\
    } \n\
\n\
    fun destroy() { } \n\
} \n\
\n\
object 'EventListFactory' \n\
{ \n\
    fun call(list) \n\
    { \n\
        return caller.spawn('EventList').__init(list); \n\
    } \n\
\n\
    fun destroy() { } \n\
} \n\
\n\
object 'EventChainFactory' \n\
{ \n\
    fun call(list) \n\
    { \n\
        return caller.spawn('EventChain').__init(list); \n\
    } \n\
\n\
    fun destroy() { } \n\
} \n\
";

/* EOF */