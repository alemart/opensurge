// -----------------------------------------------------------------------------
// File: translate_player.ss
// Description: a function object that translates the active player
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;

//
// Translate Player is a function object that translates the active player
// by a given offset.
//
// Arguments:
// - offset: Vector2.
//
object "Translate Player"
{
    fun call(offset)
    {
        player = Player.active;
        player.transform.translate(offset);
    }
}