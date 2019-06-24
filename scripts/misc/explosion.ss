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
    public zindex = 0.51;

    actor = Actor("Explosion");
    silence = false;

    state "main"
    {
        // play sound
        if(!silence) {
            sfx = Sound("samples/destroy.wav");
            sfx.play();
        }

        // adjust zindex
        actor.zindex = zindex;

        // change state
        state = "exploding";
    }

    state "exploding"
    {
        if(actor.animation.finished)
            destroy();
    }



    // --- MODIFIERS ---

    // disable the default audio
    fun mute()
    {
        silence = true;
        return this;
    }
}