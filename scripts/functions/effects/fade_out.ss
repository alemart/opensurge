// -----------------------------------------------------------------------------
// File: fade_out.ss
// Description: a function object that performs a fade-out effect
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;

//
// Fade Out is a function object that performs a
// fade-out effect.
//
// Arguments:
// - duration: number. The duration of the effect,
//                     in seconds (example: 0.5)
//
object "Fade Out"
{
    fader = null;

    fun call(duration)
    {
        if(duration > 0) {
            fader = fader || Level.spawn("Fader");
            fader.fadeTime = duration;
            fader.fadeOut();
        }
    }
}