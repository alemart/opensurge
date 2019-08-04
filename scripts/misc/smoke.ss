// -----------------------------------------------------------------------------
// File: smoke.ss
// Description: smoke effect
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;

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
        actor.zindex = 0.51;
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