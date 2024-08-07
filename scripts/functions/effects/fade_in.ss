// -----------------------------------------------------------------------------
// File: fade_in.ss
// Description: a function object that performs a fade-in effect
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;

//
// Fade In is a function object that performs a
// fade-in effect.
//
// Arguments:
// - duration: number. The duration of the effect,
//                     in seconds (example: 0.5)
//
object "Fade In"
{
    fader = null;

    fun call(duration)
    {
        if(duration > 0) {
            fader = fader || Level.spawn("Fader");
            fader.fadeTime = duration;
            fader.fadeIn();
        }
    }
}