// -----------------------------------------------------------------------------
// File: hit_player.ss
// Description: a function object that hits the player
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;

//
// Hit Player is a function object that hits
// the active player.
//
object "Hit Player"
{
    fun call()
    {
        Player.active.hit();
    }
}