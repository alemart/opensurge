// -----------------------------------------------------------------------------
// File: explosion.ss
// Description: explosion script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Transform;

object "Explosion" is "entity", "private", "disposable"
{
    actor = Actor("Explosion");
    transform = Transform();

    state "main"
    {
        if(actor.animation.finished)
            destroy();
    }

    fun constructor() { actor.zindex = 0.51; }

    // at()
    // positions the explosion (position is a Vector2 object)
    // How to use: Level.spawn("explosion").at(position);
    fun at(position)
    {
        transform.position = position;
        return this;
    }
}