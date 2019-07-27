// -----------------------------------------------------------------------------
// File: kill_player.ss
// Description: a function object that kills the player
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;

//
// Kill Player is a function object that kills
// the active player.
//
object "Kill Player"
{
    fun call()
    {
        Player.active.kill();
    }
}