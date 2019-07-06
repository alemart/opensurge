// -----------------------------------------------------------------------------
// File: none.ss
// Description: companion object for playable character 'None'
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;

object "None"
{
    player = Player("None");

    state "main"
    {
        // disable physics
        player.frozen = true;
        Console.print(player.name);
    }
}