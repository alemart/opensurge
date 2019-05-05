// -----------------------------------------------------------------------------
// File: surgeengine.ss
// Description: SurgeEngine Plugin
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
//
//                           uuuuuuuuuuuuuuuuuuuu                                    
//                         u" uuuuuuuuuuuuuuuuuu "u                                  
//                       u" u####################u "u 
//                     u" u########################u "u                              
//                   u" u############################u "u                            
//                 u" u################################u "u                          
//               u" u####################################u "u                        
//               # ######################################## #                        
//               # ######################################## #                        
//               # ###" ... "#...  ...#" ... "###  ... "### #                        
//               # ###u `"#######  ###  #####  ##  ###  ### #                        
//               # ######uu "####  ###  #####  ##  """ u### #                        
//               # ###""###  ####  ###u "###" u##  ######## #                        
//               # ####....,#####..#####....,####..######## #                        
//               # ######################################## #                        
//               "u "####################################" u"                        
//                 "u "################################" u"                          
//                   "u "############################" u"                            
//                     "u "########################" u"                              
//                       "u "####################" u"                                
//                         "u """""""""""""""""" u"                                  
//                           """"""""""""""""""""             
//               
//                      DO NOT MODIFY THE CODE BELOW!!!
//                   unless you know what you are doing ;)
//
using SurgeEngine;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;

@Plugin
object "SurgeEngine"
{
    actorFactory = spawn("ActorFactory");
    brickFactory = spawn("BrickFactory");
    inputFactory = spawn("InputFactory");
    levelManager = spawn("LevelManager");
    camera = spawn("Camera");
    collisions = spawn("Collision");
    userInterface = spawn("UI");
    audio = spawn("Audio");
    video = spawn("Video");
    prefs = spawn("Prefs");
    web = spawn("Web");
    transformFactory = spawn("TransformFactory");
    vectorFactory = spawn("VectorFactory");

    fun get_Actor()
    {
        return actorFactory;
    }

    fun get_Brick()
    {
        return brickFactory;
    }   

    fun get_Input()
    {
        return inputFactory;
    }

    fun get_LevelManager()
    {
        return levelManager;
    }

    fun get_Level()
    {
        return levelManager.currentLevel;
    }

    fun get_Camera()
    {
        return camera;
    }

    fun get_Collisions()
    {
        return collisions;
    }

    fun get_Prefs()
    {
        return prefs;
    }

    fun get_Web()
    {
        return web;
    }

    fun get_Video()
    {
        return video;
    }

    fun get_Player()
    {
        return levelManager.playerManager;
    }

    fun get_UI()
    {
        return userInterface;
    }

    fun get_Audio()
    {
        return audio;
    }

    fun get_Transform()
    {
        return transformFactory;
    }

    fun get_Vector2()
    {
        return vectorFactory;
    }
}





// -----------------------------------------------------------------------------
// Factory methods
// -----------------------------------------------------------------------------

object "ActorFactory"
{
    fun call(spriteName)
    {
        actor = caller.spawn("Actor");
        actor.__init(spriteName);
        return actor;
    }

    fun spawn(x) { return null; }
    fun destroy() { }
}

object "BrickFactory"
{
    fun call(spriteName)
    {
        brick = caller.spawn("Brick");
        brick.__init(spriteName);
        return brick;
    }

    fun destroy() { }
}

object "SensorFactory"
{
    obstacleMap = spawn("ObstacleMap");

    fun call(x, y, w, h)
    {
        if(Math.min(w, h) == 1) {
            sensor = caller.spawn("Sensor");
            sensor.__init(x, y, x + w - 1, y + h - 1, obstacleMap);
            return sensor;
        }
        else {
            message = "Invalid sensor dimensions for ";
            Application.crash(message + caller.__name);
            return null;
        }
    }

    fun destroy() { }
}

object "InputFactory"
{
    mouse = spawn("Mouse");

    fun call(inputMap)
    {
        input = caller.spawn("Input");
        input.__init(inputMap);
        return input;
    }

    fun get_Mouse()
    {
        return mouse;
    }

    fun destroy() { }
}

object "Collision"
{
    manager = spawn("CollisionManager");
    box = spawn("CollisionBoxFactory");
    ball = spawn("CollisionBallFactory");
    sensor = spawn("SensorFactory");

    fun get_CollisionBox()
    {
        return box;
    }

    fun get_CollisionBall()
    {
        return ball;
    }

    fun get_Sensor()
    {
        return sensor;
    }

    fun CollisionBox(width, height)
    {
        return box(width, height);
    }

    fun CollisionBall(radius)
    {
        return ball(radius);
    }

    fun Sensor(x, y, w, h)
    {
        return sensor(x, y, w, h);
    }

    fun destroy() { }
}

object "CollisionBoxFactory"
{
    manager = parent.child("CollisionManager");

    fun call(width, height)
    {
        return __spawn(caller, width, height);
    }

    fun __spawn(parnt, width, height) // called by Player
    {
        collider = parnt.spawn("CollisionBox");
        collider.__init(manager, width, height);
        return collider;
    }

    fun destroy() { }
}

object "CollisionBallFactory"
{
    manager = parent.child("CollisionManager");

    fun call(radius)
    {
        collider = caller.spawn("CollisionBall");
        collider.__init(manager, radius);
        return collider;
    }

    fun destroy() { }
}

object "UI"
{
    text = spawn("TextFactory");

    fun get_Text()
    {
        return text;
    }

    fun Text(fontName)
    {
        return text(fontName);
    }

    fun destroy() { }
}

object "TextFactory"
{
    fun call(fontName)
    {
        text = caller.spawn("Text");
        text.__init(fontName);
        return text;
    }

    fun destroy() { }
}

object "TransformFactory"
{
    fun call()
    {
        t2 = caller.child("Transform2D");
        return t2 != null ? t2 : caller.spawn("Transform2D");
    }
    
    fun destroy() { }
}

object "VectorFactory"
{
    temp = System.child("__Temp");
    up = spawn("Vector2");
    right = spawn("Vector2");
    down = spawn("Vector2");
    left = spawn("Vector2");
    zero = spawn("Vector2");

    fun call(x, y)
    {
        v = temp.spawn("Vector2");
        v.__init(x, y);
        return v;
    }

    fun constructor()
    {
        up.__init(0, -1);
        right.__init(1, 0);
        down.__init(0, 1);
        left.__init(-1, 0);
        zero.__init(0, 0);
    }

    fun get_up()
    {
        return up;
    }

    fun get_right()
    {
        return right;
    }

    fun get_down()
    {
        return down;
    }

    fun get_left()
    {
        return left;
    }

    fun get_zero()
    {
        return zero;
    }
    
    fun destroy() { }
}

object "Audio"
{
    music = spawn("MusicFactory");
    sound = spawn("SoundFactory");

    fun get_Music()
    {
        return music;
    }

    fun get_Sound()
    {
        return sound;
    }

    fun destroy() { }
}

object "MusicFactory"
{
    fun call(pathToMusic)
    {
        return __spawn(caller, pathToMusic);
    }

    fun __spawn(parnt, pathToMusic) // called by Level
    {
        music = parnt.spawn("Music");
        music.__init(pathToMusic);
        return music;
    }

    fun destroy() { }
}

object "SoundFactory"
{
    fun call(pathToSound)
    {
        sound = caller.spawn("Sound");
        sound.__init(pathToSound);
        return sound;
    }

    fun destroy() { }
}

object "Video"
{
    screen = spawn("Screen");

    fun get_Screen()
    {
        return screen;
    }

    fun destroy() { }
}



// -----------------------------------------------------------------------------
// Player API
// -----------------------------------------------------------------------------

object "PlayerManager"
{
    // get player by name or id
    // display an error if not found
    fun call(nameOrId)
    {
        if(typeof(nameOrId) == "string") {
            if((player = __getByName(nameOrId)) != null)
                return player;
        }
        else if(typeof(nameOrId) == "number") {
            if((player = __getById(nameOrId)) != null)
                return player;
        }

        // error
        Application.crash("Can't find Player \"" + nameOrId + "\": no such player in the scene.");
        return null;
    }

    // get player by id
    // returns null if not found
    fun get(playerId)
    {
        return __getById(playerId);
    }

    // does the given player exist in the current scene?
    fun exists(playerName)
    {
        return __getByName(playerName) != null;
    }

    // fun __spawnPlayers() { [builtin] }
    // fun __getById(id) { [builtin] }
    // fun __getByName(name) { [builtin] }
    // fun get_active() { [builtin] }
    // fun get_count() { [builtin] }

    // manager overrides
    fun destroy() { }
}