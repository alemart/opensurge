// -----------------------------------------------------------------------------
// File: unfreeze_player.ss
// Description: a function object that unfreezes the player
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;

//
// Unfreeze Player is a function object that
// unfreezes the active player (enables physics)
//
object "Unfreeze Player"
{
    fun call()
    {
        Player.active.frozen = false;
    }
}