//
// File: collisions.ss
// Description: collision system (scripting only)
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

object "CollisionManager"
{
    colliders = [];

    state "main"
    {
        // check for collisions
        foreach(collider in colliders) {
            foreach(c in colliders) {
                if(c != collider && collider.overlaps(c))
                    collider.__notify(c);
            }
        }

        // clear the current colliders
        while(colliders.pop());
    }

    fun __notify(collider)
    {
        colliders.push(collider);
    }
}

// Axis-aligned collision box
// To be spawned as a child of an entity
object "CollisionBox" is "Collider"
{
    manager = null;
    width = 1;
    height = 1;
    transform = spawn("Transform2D");
    collisionFlags = 0;
    debug = false;

    state "main"
    {
        // I will notify the manager only if I'm active
        manager.__notify(this);
    }

    fun get_width()
    {
        return width;
    }

    fun get_height()
    {
        return height;
    }

    fun get_left()
    {
        return transform.worldX - width / 2;
    }

    fun get_right()
    {
        return transform.worldX + width / 2;
    }

    fun get_top()
    {
        return transform.worldY - height / 2;
    }

    fun get_bottom()
    {
        return transform.worldY + height / 2;
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
        return parent;
    }

    // setAnchor(x, y)
    // sets the anchor of the collider to a certain position (x,y),
    // where 0 <= x, y <= 1. Defaults to (0.5, 0.5), the center
    // of the collider
    // Note: the anchor will be aligned to the hot_spot of the entity
    fun setAnchor(x, y)
    {
        transform.xpos = (0.5 - x) * width;
        transform.ypos = (0.5 - y) * height;
    }

    // overlaps()
    // Returns true if this collider overlaps with the given collider
    fun overlaps(collider)
    {
        return
            (this.left < collider.right && this.right > collider.left) && 
            (this.top < collider.bottom && this.bottom > collider.top)
        ;
    }

    fun __init(mgr, w, h)
    {
        // initialization
        manager = mgr;
        width = Math.max(w, 1); // read-only
        height = Math.max(h, 1);

        // validation
        if(!parent.hasTag("entity"))
            Application.crash("Object \"" + parent.__name + "\" (parent of " + __name + ") must be an entity.");
        if(parent.hasFunction("onCollision"))
            collisionFlags = 1;
    }

    // the manager is telling us about a collision somewhere
    fun __notify(otherCollider)
    {
        if(collisionFlags > 0)
            parent.onCollision(otherCollider);
    }
}