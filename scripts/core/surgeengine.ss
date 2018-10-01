//
// File: surgeengine.ss
// Description: SurgeEngine Plugin
// Copyright (C) 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
// http://opensurge2d.org
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//             uuuuuuuuuuuuuuuuuuuu                                    
//           u" uuuuuuuuuuuuuuuuuu "u                                  
//         u" u####################u "u 
//       u" u########################u "u                              
//     u" u############################u "u                            
//   u" u################################u "u                          
// u" u####################################u "u                        
// # ######################################## #                        
// # ######################################## #                        
// # ###" ... "#...  ...#" ... "###  ... "### #                        
// # ###u `"#######  ###  #####  ##  ###  ### #                        
// # ######uu "####  ###  #####  ##  """ u### #                        
// # ###""###  ####  ###u "###" u##  ######## #                        
// # ####....,#####..#####....,####..######## #                        
// # ######################################## #                        
// "u "####################################" u"                        
//   "u "################################" u"                          
//     "u "############################" u"                            
//       "u "########################" u"                              
//         "u "####################" u"                                
//           "u """""""""""""""""" u"                                  
//             """"""""""""""""""""             
// 
//        DO NOT MODIFY THE CODE BELOW!!!
//     unless you know what you are doing ;)
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
        collider = caller.spawn("CollisionBox");
        collider.__init(manager, width, height);
        return collider;
    }
}