// -----------------------------------------------------------------------------
// File: smoke.ss
// Description: smoke effect
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;
using SurgeEngine.Behaviors.DirectionalMovement;

// Smoke: used when braking
object "Smoke" is "entity", "private", "disposable"
{
    actor = Actor("Smoke");
    transform = Transform();

    state "main"
    {
        if(actor.animation.finished)
            destroy();
    }

    fun setScale(scale)
    {
        transform.localScale = Vector2(scale, scale);
        return this;
    }

    fun setOffset(x, y)
    {
        actor.offset = Vector2(x, y);
        return this;
    }

    fun constructor()
    {
        actor.zindex = 0.6;
    }
}

// Speed Smoke: used when dashing
object "Speed Smoke" is "entity", "private", "disposable"
{
    actor = Actor("Speed Smoke");
    transform = Transform();

    state "main"
    {
        if(actor.animation.finished)
            destroy();
    }

    fun setOffset(x, y)
    {
        actor.offset = Vector2(x, y);
        return this;
    }

    fun setDirection(direction)
    {
        actor.hflip = (direction < 0);
        return this;
    }

    fun setScale(scale)
    {
        transform.localScale = Vector2(scale, scale);
        return this;
    }

    fun constructor()
    {
        actor.zindex = 0.5;
    }
}

// Mini Smoke: used when dashing
object "Mini Smoke" is "entity", "private", "disposable"
{
    actor = Actor("Mini Smoke");
    movement = DirectionalMovement();

    state "main"
    {
        if(actor.animation.finished)
            destroy();
    }

    // velocity must be a Vector2 object
    fun setVelocity(velocity)
    {
        movement.direction = velocity.normalized();
        movement.speed = velocity.length;
        return this;
    }

    fun constructor()
    {
        movement.speed = 0;
        //actor.zindex = 0.51;
    }
}