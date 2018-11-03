// -----------------------------------------------------------------------------
// File: collisions.ss
// Description: collision system (scripting only)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

object "CollisionManager"
{
    colliders = [];

    state "main"
    {
        // check for collisions
        while(collider = colliders.pop()) {
            foreach(c in colliders) {
                if(collider.collidesWith(c)) {
                    collider.__notify(c);
                    c.__notify(collider);
                }
            }
        }
    }

    fun __notify(collider)
    {
        colliders.push(collider);
    }
}

// Axis-aligned collision box
// To be spawned as a child of an entity
object "CollisionBox" is "collider"
{
    manager = null;
    debug = false;
    width = 1;
    height = 1;
    worldX = 0;
    worldY = 0;
    transform = spawn("Transform2D");
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

    fun get_width()
    {
        return width;
    }

    fun set_width(w)
    {
        width = Math.max(1, w);
    }

    fun get_height()
    {
        return height;
    }

    fun set_height(h)
    {
        height = Math.max(1, h);
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

    // get_debug()
    // Is debugging enabled?
    fun get_debug()
    {
        return debug;
    }

    // set_debug()
    // Display the collider for debugging purposes
    fun set_debug(dbg)
    {
        debug = dbg;
    }

    // get_entity()
    // the entity associated with this collider
    fun get_entity()
    {
        return entity;
    }

    // setAnchor(x, y)
    // sets the anchor of the collider to a certain position (x,y),
    // where 0 <= x, y <= 1. Defaults to (0.5, 0.5), the center of
    // the collider. (0,0) is the top-left; (1,1), the bottom-right
    // Note: the anchor will be aligned to the hot_spot of the entity
    fun setAnchor(x, y)
    {
        transform.xpos = (0.5 - x) * width;
        transform.ypos = (0.5 - y) * height;
        __updateWorldCoordinates();
    }

    // collidesWith()
    // Returns true if this collider collides with the given collider
    fun collidesWith(collider)
    {
        return
            (this.left < collider.right && this.right > collider.left) && 
            (this.top < collider.bottom && this.bottom > collider.top)
        ;
    }

    // constructor()
    // Object constructor
    fun constructor()
    {
        // entity validation
        entity = parent;
        if(!entity.hasTag("entity"))
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
        worldX = transform.worldX;
        worldY = transform.worldY;
    }
}