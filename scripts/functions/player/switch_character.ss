// -----------------------------------------------------------------------------
// File: switch_character.ss
// Description: a function object that switches the character
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;

//
// Switch Character is a function object that switches
// the character (i.e., changes the active player)
//
object "Switch Character"
{
    fun call(playerName)
    {
        player = Player(playerName);
        if(player != null)
            player.focus();
    }
}