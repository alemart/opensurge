// -----------------------------------------------------------------------------
// File: play_level_music.ss
// Description: a function object that plays the Level Music
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;

//
// Play Level Music is a function object that
// plays the Level Music.
//
object "Play Level Music"
{
    fun call()
    {
        if(!Level.music.playing)
            Level.music.play();
    }
}