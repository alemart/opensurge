// -----------------------------------------------------------------------------
// File: play_boss_music.ss
// Description: a function object that plays the Boss Music
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;

//
// Play Boss Music is a function object that
// plays the Boss Music.
//
object "Play Boss Music"
{
    music = Level.child("Boss Music") || Level.spawn("Boss Music");

    fun call()
    {
        if(!music.playing)
            music.play();
    }
}