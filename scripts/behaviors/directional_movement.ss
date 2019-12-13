// -----------------------------------------------------------------------------
// File: direcional_movement.ss
// Description: directional movement behavior
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*
 * This behavior makes the entity perform a Directional
 * Movement on the 2D plane. It has a direction vector
 * and a speed, a scalar value given in pixels per second.
 *
 * You may control the direction of the movement using
 * a vector or an angle given in degrees. Additionally,
 * you may control the speed of the movement by
 * changing the speed property directly.
 *
 * DirectionalMovement is very versatile; for example,
 * you can implement baddies of many types, flying
 * objects, and even racing cars viewed from a
 * top-down view. Your imagination is the limit!
 *
 * Example:
 *

using SurgeEngine.Actor;
using SurgeEngine.Vector2;
using SurgeEngine.Behaviors.DirectionalMovement;

object "Simple Ball" is "entity"
{
    actor = Actor("Simple Ball");
    movement = DirectionalMovement();

    state "main"
    {
        movement.speed = 60;
        movement.direction = Vector2.right;
    }
}

 *
 * It's amazing!
 *
 */

// -----------------------------------------------------------------------------
// The following code makes DirectionalMovement work.
// Think twice before modifying this code!
// -----------------------------------------------------------------------------
using SurgeEngine.Vector2;

object "DirectionalMovement" is "behavior"
{
    public readonly entity = parent; // the entity associated with this behavior
    public speed = 0; // speed in px/s

    ccwAngle = 0; // counterclockwise angle
    direction = Vector2.right; // initial direction: "forward"
    normalizedDirection = Vector2.right;
    transform = null;

    state "main"
    {
        // setup
        if(transform == null)
            transform = entity.child("Transform") || entity.spawn("Transform");

        // move
        if(speed != 0) {
            dt = Time.delta;
            transform.translateBy(speed * normalizedDirection.x * dt, speed * normalizedDirection.y * dt);
        }
    }

    state "disabled"
    {
        // no movement
    }

    // constructor
    fun constructor()
    {
        // behavior validation
        if(!entity.hasTag("entity"))
            Application.crash("Object \"" + entity.__name + "\" must be tagged \"entity\" to use " + this.__name + ".");
    }



    // --- PROPERTIES ---

    // get/set the direction vector
    fun get_direction()
    {
        return direction;
    }

    fun set_direction(newDirection)
    {
        if(direction != newDirection) {
            direction = newDirection;
            normalizedDirection = direction.normalized();
            ccwAngle = 360 - direction.angle;
        }
    }

    // get/set the counterclockwise angle of the direction vector
    // 0 means right, 90 means up, etc.
    fun get_angle()
    {
        return ccwAngle;
    }

    fun set_angle(newAngle)
    {
        if(!Math.approximately(ccwAngle, newAngle)) {
            this.direction = Vector2.right.rotatedBy(-ccwAngle);
            ccwAngle = newAngle;
        }
    }

    // enable/disable movement
    fun get_enabled()
    {
        return state != "disabled";
    }

    fun set_enabled(enabled)
    {
        state = enabled ? "main" : "disabled";
    }
}