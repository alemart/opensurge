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
    inputFactory = spawn("InputFactory");
    levelManager = spawn("LevelManager");
    camera = spawn("Camera");
    collisions = spawn("Collision");
    userInterface = spawn("UI");
    audio = spawn("Audio");
    video = spawn("Video");
    prefs = spawn("Prefs");
    transformFactory = spawn("TransformFactory");
    vectorFactory = spawn("VectorFactory");

    fun get_Actor()
    {
        return actorFactory;
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
    children = null; // cached value

    // get player by name or id
    // just an alias
    fun call(nameOrId)
    {
        if(typeof(nameOrId) == "string")
            return __getByName(nameOrId);
        else if(typeof(nameOrId) == "number")
            return __getById(nameOrId);
        else
            return __error("Can't find Player \"" + nameOrId + "\": invalid identifier.");
    }

    // the number of players
    fun get_count()
    {
        return this.childCount;
    }

    // __error()
    // display error message
    fun __error(message)
    {
        Application.crash(message);
        return null;
    }

    // __getByName()
    // get player by name; will crash if not found
    fun __getByName(name)
    {
        cnt = this.count;
        for(i = 0; i < cnt; i++) {
            player = __getById(i);
            if(player.name == name)
                return player;
        }
        return __error("Can't find Player \"" + name + "\": no such character.");
    }

    // __getById()
    // get player by id; will crash if not found
    fun __getById(playerID)
    {
        id = Number(playerID);
        if(id.isInteger()) {
            if(id >= 0 && id < this.childCount) {
                if(children == null)
                    children = this.__children;
                return children[id];
            }
        }
        return __error("Can't find Player " + playerID + ": invalid id.");
    }

    // fun __spawnPlayers() { [builtin] }
    // fun get_active() { [builtin] }

    // manager overrides
    fun destroy() { }
}



// -----------------------------------------------------------------------------
// Collision API
// -----------------------------------------------------------------------------

// CollisionManager
// detects collisions between colliders
object "CollisionManager"
{
    colliders = [];

    state "main"
    {
        // check for collisions
        while(collider = colliders.pop()) {
            length = colliders.length;
            for(i = 0; i < length; i++) {
                otherCollider = colliders[i];
                if(collider.collidesWith(otherCollider)) {
                    collider.__notify(otherCollider);
                    otherCollider.__notify(collider);
                }
            }
        }
    }

    // a collider is available in this frame
    fun __notify(collider)
    {
        colliders.push(collider);
    }

    // can't destroy this
    fun destroy() { }
}

// Axis-aligned collision box
// To be spawned as a child of an entity
object "CollisionBox" is "collider"
{
    manager = null;
    visible = false;
    width = 1;
    height = 1;
    worldX = 0;
    worldY = 0;
    //center = Vector2.zero;
    transform = Transform();
    collisionFlags = 0;
    entity = null;
    prevCollisions = [];
    currCollisions = [];

    state "main"
    {
        // update world coordinates
        __updateWorldCoordinates();

        // update collisions
        while(prevCollisions.pop());
        while(collider = currCollisions.pop())
            prevCollisions.push(collider);

        // I will notify the manager only if I'm active
        manager.__notify(this);
    }

    fun get_left()
    {
        return worldX - width / 2;
    }

    fun get_right()
    {
        return worldX + width / 2;
    }

    fun get_top()
    {
        return worldY - height / 2;
    }

    fun get_bottom()
    {
        return worldY + height / 2;
    }

    // get_width()
    // The width of the collider
    fun get_width()
    {
        return width;
    }

    // set_width()
    // Set the width of the collider
    fun set_width(w)
    {
        width = Math.max(1, w);
    }

    // get_height()
    // The height of the collider
    fun get_height()
    {
        return height;
    }

    // set_height()
    // Set the height of the collider    
    fun set_height(h)
    {
        height = Math.max(1, h);
    }

    // get_visible()
    // Is the collider visible? Useful for debugging.
    fun get_visible()
    {
        return visible;
    }

    // set_visible()
    // Display the collider for debugging purposes
    fun set_visible(v)
    {
        visible = v;
    }

    // get_entity()
    // the entity associated with this collider
    fun get_entity()
    {
        return entity;
    }

    // get___type()
    // the type of the collider
    fun get___type()
    {
        return 1;
    }

    // collidesWith()
    // Returns true if this collider collides with the given collider
    fun collidesWith(collider)
    {
        if(collider.__type == 1) {
            return
                (this.left < collider.right && this.right > collider.left) && 
                (this.top < collider.bottom && this.bottom > collider.top)
            ;
        }
        else if(collider.__type == 2) {
            cx = collider.center.x;
            cy = collider.center.y;
            r = collider.radius;
            dx = cx - Math.clamp(cx, this.left, this.right);
            dy = cy - Math.clamp(cy, this.top, this.bottom);
            return dx * dx + dy * dy < r * r;
        }
    }

    // contains(x, y)
    // Checks if world-position (x, y) is inside the collider
    fun contains(x, y)
    {
        return !(x < this.left || x > this.right || y < this.top || y > this.bottom);
    }

    // setAnchor(x, y)
    // sets the anchor of the collider to a certain position (x,y),
    // where 0 <= x, y <= 1. Defaults to (0.5, 0.5), the center of
    // the collider. (0,0) is the top-left; (1,1), the bottom-right
    // Note: the anchor will be aligned to the hot_spot of the entity
    fun setAnchor(x, y)
    {
        transform.position = Vector2((0.5 - x) * width, (0.5 - y) * height);
        __updateWorldCoordinates();
    }

    // constructor()
    // Object constructor
    fun constructor()
    {
        // entity validation
        entity = parent;
        if(!entity.hasTag("entity") && !parent.hasTag("collider"))
            Application.crash(__name + " requires object \"" + entity.__name + "\" to be an entity.");
        else if(entity.hasTag("detached"))
            Application.crash(__name + " won't work with detached entities like \"" + entity.__name + "\".");

        // collision flags
        collisionFlags = 0;
        if(entity.hasFunction("onCollision"))
            collisionFlags += 1;
    }

    // initialization
    fun __init(mgr, w, h)
    {
        manager = mgr;
        set_width(w);
        set_height(h);
    }

    // check collision flags
    fun __bitflag(flag)
    {
        if(flag == 1) return Math.mod(collisionFlags, 2);
        if(flag == 2) return Math.mod(Math.floor(collisionFlags / 2), 2);
        if(flag == 4) return Math.mod(Math.floor(collisionFlags / 4), 2);
        return 0;
    }

    // the manager is telling us about a collision somewhere
    fun __notify(otherCollider)
    {
        currCollisions.push(otherCollider);
        if(collisionFlags > 0) {
            if(prevCollisions.indexOf(otherCollider) < 0)
                entity.onCollision(otherCollider);
        }
    }

    // cache the world coordinates (save some time)
    fun __updateWorldCoordinates()
    {
        center = transform.position;
        worldX = center.x;
        worldY = center.y;
    }
}

// Collision Ball (circle)
// To be spawned as a child of an entity
object "CollisionBall" is "collider"
{
    manager = null;
    visible = false;
    radius = 1;
    worldX = 0;
    worldY = 0;
    center = Vector2.zero;
    transform = Transform();
    collisionFlags = 0;
    entity = null;
    prevCollisions = [];
    currCollisions = [];

    state "main"
    {
        // update world coordinates
        __updateWorldCoordinates();

        // update collisions
        while(prevCollisions.pop());
        while(collider = currCollisions.pop())
            prevCollisions.push(collider);

        // I will notify the manager only if I'm active
        manager.__notify(this);
    }

    // get_center()
    // The center of the collider
    fun get_center()
    {
        return center;
    }

    // get_radius()
    // Returns the radius, in pixels
    fun get_radius()
    {
        return radius;
    }

    // set_radius()
    // Redefine the radius of the collider
    fun set_radius(r)
    {
        radius = Math.max(1, r);
    }

    // get_visible()
    // Is the collider visible? Useful for debugging.
    fun get_visible()
    {
        return visible;
    }

    // set_visible()
    // Display the collider for debugging purposes
    fun set_visible(v)
    {
        visible = v;
    }

    // get_entity()
    // the entity associated with this collider
    fun get_entity()
    {
        return entity;
    }

    // get___type()
    // the type of the collider
    fun get___type()
    {
        return 2;
    }

    // collidesWith()
    // Returns true if this collider collides with the given collider
    fun collidesWith(collider)
    {
        if(collider.__type == 1) {
            dx = worldX - Math.clamp(worldX, collider.left, collider.right);
            dy = worldY - Math.clamp(worldY, collider.top, collider.bottom);
            return dx * dx + dy * dy < radius * radius;
        }
        else if(collider.__type == 2) {
            dx = worldX - collider.center.x;
            dy = worldY - collider.center.y;
            rr = radius + collider.radius;
            return dx * dx + dy * dy < rr * rr;
        }
    }

    // contains(x, y)
    // Checks if world-position (x, y) is inside the collider
    fun contains(x, y)
    {
        dx = x - worldX;
        dy = y - worldY;
        return dx * dx + dy * dy <= radius * radius;
    }

    // setAnchor(x, y)
    // sets the anchor of the collider to a certain position (x,y),
    // where 0 <= x, y <= 1.
    fun setAnchor(x, y)
    {
        size = radius * 2;
        transform.position = Vector2((0.5 - x) * size, (0.5 - y) * size);
        __updateWorldCoordinates();
    }

    // constructor()
    // Object constructor
    fun constructor()
    {
        // entity validation
        entity = parent;
        if(!entity.hasTag("entity") && !parent.hasTag("collider"))
            Application.crash(__name + " requires object \"" + entity.__name + "\" to be an entity | collider.");
        else if(entity.hasTag("detached"))
            Application.crash(__name + " won't work with detached entities like \"" + entity.__name + "\".");

        // collision flags
        collisionFlags = 0;
        if(entity.hasFunction("onCollision"))
            collisionFlags += 1;
    }

    // __init()
    // Initialization
    fun __init(mgr, r)
    {
        manager = mgr;
        set_radius(r);
    }

    // __notify()
    // the manager is telling us about a collision somewhere
    fun __notify(otherCollider)
    {
        currCollisions.push(otherCollider);
        if(collisionFlags > 0) {
            if(prevCollisions.indexOf(otherCollider) < 0)
                entity.onCollision(otherCollider);
        }
    }

    // __updateWorldCoordinates()
    // cache the world coordinates (save some time)
    fun __updateWorldCoordinates()
    {
        center = transform.position;
        worldX = center.x;
        worldY = center.y;
    }
}