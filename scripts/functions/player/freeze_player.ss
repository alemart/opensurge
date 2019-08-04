// -----------------------------------------------------------------------------
// File: freeze_player.ss
// Description: a function object that freezes the player
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;

//
// Freeze Player is a function object that
// freezes the active player (disables physics)
//
object "Freeze Player"
{
    fun call()
    {
        Player.active.frozen = true;
    }
}