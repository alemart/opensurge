/*
 * Open Surge Engine
 * surgeengine.c - scripting system: SurgeEngine plugin
 * Copyright (C) 2018-2019  Alexandre Martins <alemartf@gmail.com>
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

/* the following code, written in SurgeScript language, is MIT-licensed */
static const char code_in_surgescript[] = "\n\
@Plugin \n\
object 'SurgeEngine' \n\
{ \n\
    actorFactory = spawn('ActorFactory'); \n\
    behaviorFactory = spawn('BehaviorFactory'); \n\
    brickFactory = spawn('BrickFactory'); \n\
    inputFactory = spawn('InputFactory'); \n\
    levelManager = spawn('LevelManager'); \n\
    camera = spawn('Camera'); \n\
    collisions = spawn('Collision'); \n\
    userInterface = spawn('UI'); \n\
    audio = spawn('Audio'); \n\
    video = spawn('Video'); \n\
    prefs = spawn('Prefs'); \n\
    lang = spawn('Lang'); \n\
    web = spawn('Web'); \n\
    transformFactory = spawn('TransformFactory'); \n\
    vectorFactory = spawn('VectorFactory'); \n\
\n\
    fun get_Actor() \n\
    { \n\
        return actorFactory; \n\
    } \n\
\n\
    fun get_Behavior() \n\
    { \n\
        return behaviorFactory; \n\
    } \n\
\n\
    fun get_Brick() \n\
    { \n\
        return brickFactory; \n\
    }    \n\
\n\
    fun get_Input() \n\
    { \n\
        return inputFactory; \n\
    } \n\
\n\
    fun get_LevelManager() \n\
    { \n\
        return levelManager; \n\
    } \n\
\n\
    fun get_Level() \n\
    { \n\
        return levelManager.currentLevel; \n\
    } \n\
\n\
    fun get_Lang() \n\
    { \n\
        return lang; \n\
    } \n\
\n\
    fun get_Camera() \n\
    { \n\
        return camera; \n\
    } \n\
\n\
    fun get_Collisions() \n\
    { \n\
        return collisions; \n\
    } \n\
\n\
    fun get_Prefs() \n\
    { \n\
        return prefs; \n\
    } \n\
\n\
    fun get_Web() \n\
    { \n\
        return web; \n\
    } \n\
\n\
    fun get_Video() \n\
    { \n\
        return video; \n\
    } \n\
\n\
    fun get_Player() \n\
    { \n\
        return levelManager.playerManager; \n\
    } \n\
\n\
    fun get_UI() \n\
    { \n\
        return userInterface; \n\
    } \n\
\n\
    fun get_Audio() \n\
    { \n\
        return audio; \n\
    } \n\
\n\
    fun get_Transform() \n\
    { \n\
        return transformFactory; \n\
    } \n\
\n\
    fun get_Vector2() \n\
    { \n\
        return vectorFactory; \n\
    } \n\
\n\
    fun get_version() { return '" GAME_VERSION_STRING "'; } \n\
    fun destroy() { } \n\
} \n\
\n\
\n\
\n\
\n\
\n\
// ----------------------------------------------------------------------------- \n\
// Factory methods \n\
// ----------------------------------------------------------------------------- \n\
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
    fun DirectionalMovement() \n\
    { \n\
        return caller.spawn('DirectionalMovement'); \n\
    } \n\
\n\
    fun Platformer() \n\
    { \n\
        return caller.spawn('PlatformMovement'); \n\
    } \n\
\n\
    fun Enemy() \n\
    { \n\
        return caller.spawn('Enemy'); \n\
    } \n\
\n\
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
    mouse = spawn('Mouse'); \n\
\n\
    fun call(inputMap) \n\
    { \n\
        input = caller.spawn('Input'); \n\
        input.__init(inputMap); \n\
        return input; \n\
    } \n\
\n\
    fun get_Mouse() \n\
    { \n\
        return mouse; \n\
    } \n\
\n\
    fun destroy() { } \n\
} \n\
\n\
object 'Collision' \n\
{ \n\
    manager = spawn('CollisionManager'); \n\
    box = spawn('CollisionBoxFactory'); \n\
    ball = spawn('CollisionBallFactory'); \n\
    sensor = spawn('SensorFactory'); \n\
\n\
    fun get_CollisionBox() \n\
    { \n\
        return box; \n\
    } \n\
\n\
    fun get_CollisionBall() \n\
    { \n\
        return ball; \n\
    } \n\
\n\
    fun get_Sensor() \n\
    { \n\
        return sensor; \n\
    } \n\
\n\
    fun CollisionBox(width, height) \n\
    { \n\
        return box(width, height); \n\
    } \n\
\n\
    fun CollisionBall(radius) \n\
    { \n\
        return ball(radius); \n\
    } \n\
\n\
    fun Sensor(x, y, w, h) \n\
    { \n\
        return sensor(x, y, w, h); \n\
    } \n\
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
    text = spawn('TextFactory'); \n\
\n\
    fun get_Text() \n\
    { \n\
        return text; \n\
    } \n\
\n\
    fun Text(fontName) \n\
    { \n\
        return text(fontName); \n\
    } \n\
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
        t2 = caller.child('Transform2D'); \n\
        return t2 != null ? t2 : caller.spawn('Transform2D'); \n\
    } \n\
     \n\
    fun destroy() { } \n\
} \n\
\n\
object 'VectorFactory' \n\
{ \n\
    temp = System.child('__Temp'); \n\
    up = spawn('Vector2'); \n\
    right = spawn('Vector2'); \n\
    down = spawn('Vector2'); \n\
    left = spawn('Vector2'); \n\
    zero = spawn('Vector2'); \n\
\n\
    fun call(x, y) \n\
    { \n\
        v = temp.spawn('Vector2'); \n\
        v.__init(x, y); \n\
        return v; \n\
    } \n\
\n\
    fun constructor() \n\
    { \n\
        up.__init(0, -1); \n\
        right.__init(1, 0); \n\
        down.__init(0, 1); \n\
        left.__init(-1, 0); \n\
        zero.__init(0, 0); \n\
    } \n\
\n\
    fun get_up() \n\
    { \n\
        return up; \n\
    } \n\
\n\
    fun get_right() \n\
    { \n\
        return right; \n\
    } \n\
\n\
    fun get_down() \n\
    { \n\
        return down; \n\
    } \n\
\n\
    fun get_left() \n\
    { \n\
        return left; \n\
    } \n\
\n\
    fun get_zero() \n\
    { \n\
        return zero; \n\
    } \n\
     \n\
    fun destroy() { } \n\
} \n\
\n\
object 'Audio' \n\
{ \n\
    music = spawn('MusicFactory'); \n\
    sound = spawn('SoundFactory'); \n\
\n\
    fun get_Music() \n\
    { \n\
        return music; \n\
    } \n\
\n\
    fun get_Sound() \n\
    { \n\
        return sound; \n\
    } \n\
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
object 'Video' \n\
{ \n\
    screen = spawn('Screen'); \n\
\n\
    fun get_Screen() \n\
    { \n\
        return screen; \n\
    } \n\
\n\
    fun destroy() { } \n\
} \n\
\n\
\n\
\n\
// ----------------------------------------------------------------------------- \n\
// Player API \n\
// ----------------------------------------------------------------------------- \n\
\n\
object 'PlayerManager' \n\
{ \n\
    // get player by name or id \n\
    fun call(nameOrId) \n\
    { \n\
        if(typeof(nameOrId) == 'string') { \n\
            if((player = __getByName(nameOrId)) != null) \n\
                return player; \n\
        } \n\
        else if(typeof(nameOrId) == 'number') { \n\
            if((player = __getById(nameOrId)) != null) \n\
                return player; \n\
        } \n\
\n\
        // error \n\
        Application.crash('Cannot find Player \"' + nameOrId + '\": no such player in the scene.'); \n\
        return null; \n\
    } \n\
\n\
    // get player by id \n\
    fun get(playerId) \n\
    { \n\
        player = __getById(playerId); \n\
        if(player == null) \n\
            Application.crash('Cannot find Player #' + playerId + ': no such player in the scene.'); \n\
\n\
        return player; \n\
    } \n\
\n\
    // does the given player exist in the current scene? \n\
    fun exists(playerName) \n\
    { \n\
        return __getByName(playerName) != null; \n\
    } \n\
\n\
    // fun __spawnPlayers() { [builtin] } \n\
    // fun __getById(id) { [builtin] } \n\
    // fun __getByName(name) { [builtin] } \n\
    // fun get_active() { [builtin] } \n\
    // fun get_count() { [builtin] } \n\
\n\
    fun destroy() { } \n\
} \n\
";

/*
 * scripting_register_surgeengine()
 * Register SurgeEngine
 */
void scripting_register_surgeengine(surgescript_vm_t* vm)
{
    surgescript_vm_compile_code_in_memory(vm, code_in_surgescript);
}