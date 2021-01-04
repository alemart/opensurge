// -----------------------------------------------------------------------------
// File: give_extra_life.ss
// Description: a function object that gives the player an extra life
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Audio.Sound;

//
// Give Extra Life is a function object that gives
// the player a single extra life.
//
object "Give Extra Life"
{
    jingle = Sound("samples/1up.ogg");

    fun call()
    {
        Player.active.lives += 1;
        jingle.play();
    }
}