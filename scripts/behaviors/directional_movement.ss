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
    public speed = 0;
    direction = Vector2.zero;
    normalizedDirection = Vector2.zero;
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

    // get/set direction
    fun get_direction()
    {
        return direction;
    }

    fun set_direction(newDirection)
    {
        if(direction != newDirection) {
            direction = newDirection;
            normalizedDirection = direction.normalized();
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

    // constructor
    fun constructor()
    {
        // validation
        if(!parent.hasTag("entity2"))
            Application.crash("The parent of " + this.__name + ", \"" + parent.__name + "\", should be tagged \"entity\".");
    }
}