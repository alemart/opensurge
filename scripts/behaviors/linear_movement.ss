// -----------------------------------------------------------------------------
// File: linear_movement.ss
// Description: linear movement behavior script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Vector2;
using SurgeEngine.Transform;

object "LinearMovement" is "behavior"
{
    public speed = 0;
    public readonly direction = Vector2.zero;
    transform = null;

    state "main"
    {
        // setup
        if(transform == null)
            transform = parent.child("Transform2D") || parent.spawn("Transform2D");

        // move
        dt = Time.delta;
        transform.move(speed * direction.x * dt, speed * direction.y * dt);
    }

    state "stopped"
    {
    }

    // set direction: normalize the vector
    fun set_direction(newDirection)
    {
        direction = newDirection.normalized();
    }

    // start the movement
    fun start()
    {
        state = "main";
    }

    // stop the movement
    fun stop()
    {
        state = "stopped";
    }

    // constructor
    fun constructor()
    {
        // validation
        if(!parent.hasTag("entity"))
            Application.crash("The parent of " + this.__name + " behavior, \"" + parent.__name + "\", should be tagged \"entity\".");
    }

    // --- MODIFIERS ---

    // set the speed of the linear movement   
    fun setSpeed(newSpeed)
    {
        speed = newSpeed;
        return this;
    }

    // set the direction of the linear movement
    fun setDirection(newDirection)
    {
        set_direction(newDirection);
        return this;
    }
}