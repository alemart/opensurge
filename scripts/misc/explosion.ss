// -----------------------------------------------------------------------------
// File: explosion.ss
// Description: explosion script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Audio.Sound;
using SurgeEngine.Actor;

object "Explosion" is "entity", "private", "disposable"
{
    actor = Actor("Explosion");
    silence = false;

    state "main"
    {
        // play sound
        if(!silence) {
            sfx = Sound("samples/destroy.wav");
            sfx.play();
        }

        // change state
        state = "exploding";
    }

    state "exploding"
    {
        if(actor.animation.finished)
            destroy();
    }

    fun constructor()
    {
        actor.zindex = 0.51;
    }



    // --- MODIFIERS ---

    // disable the default audio
    fun mute()
    {
        silence = true;
        return this;
    }
}