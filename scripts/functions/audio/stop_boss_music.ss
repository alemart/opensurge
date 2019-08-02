// -----------------------------------------------------------------------------
// File: stop_boss_music.ss
// Description: a function object that stops the Boss Music
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;

//
// Stop Boss Music is a function object that
// stops the Boss Music.
//
object "Stop Boss Music"
{
    music = Level.child("Boss Music") || Level.spawn("Boss Music");
    
    fun call()
    {
        if(music.playing)
            music.stop();
    }
}