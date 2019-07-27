// -----------------------------------------------------------------------------
// File: play_sound.ss
// Description: a function object that plays a sound
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Audio.Sound;

//
// Play Sound is a function object that plays a sound.
//
// Arguments:
// - path: string. The path of the sound file to be played
//                 (e.g., "samples/collectible.wav")
//
object "Play Sound"
{
    fun call(path)
    {
        sfx = Sound(path);
        sfx.play();
    }
}