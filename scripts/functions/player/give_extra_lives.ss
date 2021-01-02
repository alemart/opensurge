// -----------------------------------------------------------------------------
// File: give_extra_lives.ss
// Description: a function object that gives extra lives
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Audio.Music;

//
// Give Extra Lives is a function object that gives
// the player extra lives.
//
// Arguments:
// - lives: number. A positive integer.
//
object "Give Extra Lives"
{
    jingle = Music("musics/1up.ogg");

    fun call(lives)
    {
        if(lives > 0) {
            Player.active.lives += lives;
            jingle.play();
        }
    }
}