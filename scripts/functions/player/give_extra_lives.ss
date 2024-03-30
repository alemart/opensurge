// -----------------------------------------------------------------------------
// File: give_extra_lives.ss
// Description: a function object that gives extra lives
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;
using SurgeEngine.Player;
using SurgeEngine.Audio.Sound;

//
// Give Extra Lives is a function object that gives
// the player extra lives.
//
// Arguments:
// - lives: number. A positive integer.
//
object "Give Extra Lives"
{
    jingle = Sound("samples/1up.ogg");
    volumeReducer = Level.child("Reduce Music Volume Temporarily") ||
                    Level.spawn("Reduce Music Volume Temporarily");

    fun call(lives)
    {
        if(lives > 0) {
            Player.active.lives += lives;
            jingle.play();
            volumeReducer.call(jingle);
        }
    }
}

// This object is used to reduce the volume
// of the level music while we play the 1up jingle
object "Reduce Music Volume Temporarily"
{
    //
    // We'll set the volume of the level music
    // to reducedVolume while we play the jingle.
    // Next, we'll gradually increase the volume of
    // the level music back to normalVolume.
    //
    jingle = null; // sound effect
    normalVolume = 1.0; // must be 1 (what if multiple jingles play simultaneously?)
    reducedVolume = 0.3; // a value that sounds good
    transitionDuration = 0.5; // in seconds
    t = 0.0; // used to control the time

    state "main"
    {
    }

    state "reduced volume"
    {
        if(!jingle.playing) {
            t = 0;
            state = "increasing volume";
        }
    }

    state "increasing volume"
    {
        t += Time.delta;
        Level.music.volume = Math.lerp(reducedVolume, normalVolume, t / transitionDuration);
        if(t >= transitionDuration)
            state = "main";
    }

    fun call(sfx)
    {
        jingle = sfx;
        Level.music.volume = reducedVolume;
        state = "reduced volume";
    }
}