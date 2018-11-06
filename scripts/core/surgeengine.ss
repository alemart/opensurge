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

@Plugin
object "SurgeEngine"
{
    actorFactory = spawn("ActorFactory");
    inputFactory = spawn("InputFactory");
    sensorFactory = spawn("SensorFactory");
    levelManager = spawn("LevelManager");
    camera = spawn("Camera");
    mouse = spawn("Mouse");
    collisions = spawn("CollisionSystem");
    prefs = spawn("Prefs");

    fun get_Actor()
    {
        return actorFactory;
    }

    fun get_Sensor()
    {
        return sensorFactory;
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

    fun get_Mouse()
    {
        return mouse;
    }

    fun get_Collisions()
    {
        return collisions;
    }

    fun get_Prefs()
    {
        return prefs;
    }

    fun get_Player()
    {
        return levelManager.playerManager;
    }
}

object "ActorFactory"
{
    fun call(spriteName)
    {
        actor = caller.spawn("Actor");
        actor.__sprite = spriteName;
        return actor;
    }
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
}

object "InputFactory"
{
    fun call(inputMap)
    {
        input = caller.spawn("Input");
        input.__init(inputMap);
        return input;
    }
}

object "PlayerManager"
{
    children = null; // cached value: this.__children

    // get player by name
    // just an alias
    fun call(name)
    {
        return getByName(name);
    }

    // get the active player
    fun get_active()
    {
        return getByName(__activePlayerName);
    }

    // get player by name
    // will crash if not found
    fun getByName(name)
    {
        cnt = this.count;
        for(i = 0; i < cnt; i++) {
            player = getById(i);
            if(player.name == name)
                return player;
        }
        Application.crash("Can't find Player \"" + name + "\": no such character.");
        return null;
    }

    // get player by id
    // will crash if not found
    fun getById(playerID)
    {
        id = Number(playerID);
        if(id.isInteger()) {
            if(id >= 0 && id < this.__childCount) {
                if(children == null)
                    children = this.__children;
                return children[id];
            }
        }
        Application.crash("Can't find Player " + playerID + ": invalid id.");
        return null;
    }

    // the number of players
    fun get_count()
    {
        return this.__childCount;
    }

    // fun __spawnPlayers() { [builtin] }
    // fun get___activePlayerName() { [builtin] }
}

object "CollisionSystem"
{
    manager = spawn("CollisionManager");
    collisionBox = spawn("CollisionBoxFactory");

    state "main"
    {
    }

    fun get_CollisionBox()
    {
        return collisionBox;
    }
}

object "CollisionBoxFactory"
{
    manager = parent.child("CollisionManager");

    fun call(width, height)
    {
        return __create(caller, width, height);
    }

    fun __create(parnt, width, height)
    {
        collider = parnt.spawn("CollisionBox");
        collider.__init(manager, width, height);
        return collider;
    }
}