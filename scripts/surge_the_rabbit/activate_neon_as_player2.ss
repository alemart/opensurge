// -----------------------------------------------------------------------------
// File: activate_neon_as_player2.ss
// Description: level setup object used to activate Neon as Player 2
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeTheRabbit.Settings;

object "Activate Neon as Player 2" is "setup"
{
    targetName = "Neon as Player 2";

    fun constructor()
    {
        if(!Settings.wantNeonAsPlayer2)
            return;

        for(i = 0; i < Player.count; i++) {
            player = Player[i];
            if(player.name == "None") {
                if(player.transformInto(targetName))
                    return;
                else
                    break;
            }
        }

        Console.print(this.__name + ": error");
    }
}