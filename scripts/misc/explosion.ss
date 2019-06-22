// -----------------------------------------------------------------------------
// File: explosion.ss
// Description: explosion script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;

object "Explosion" is "entity", "private", "disposable"
{
    actor = Actor("Explosion");

    state "main"
    {
        if(actor.animation.finished)
            destroy();
    }

    fun constructor()
    {
        actor.zindex = 0.51;
    }
}