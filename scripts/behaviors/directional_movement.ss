// -----------------------------------------------------------------------------
// File: direcional_movement.ss
// Description: directional movement behavior
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Vector2;
using SurgeEngine.Transform;

object "DirectionalMovement" is "behavior"
{
    public speed = 0; // speed in px/s
    ccwAngle = 0; // counterclockwise angle
    direction = Vector2.right; // initial direction: "forward"
    normalizedDirection = Vector2.right;
    transform = null;

    state "main"
    {
        // setup
        if(transform == null)
            transform = parent.child("Transform2D") || parent.spawn("Transform2D");

        // move
        if(speed != 0) {
            dt = Time.delta;
            transform.move(speed * normalizedDirection.x * dt, speed * normalizedDirection.y * dt);
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
        if(!parent.hasTag("entity"))
            Application.crash("Object \"" + parent.__name + "\" must be tagged \"entity\" to use " + this.__name + ".");
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