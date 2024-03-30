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
    fun constructor()
    {
        if(!wantNeon())
            return;

        for(i = 0; i < Player.count; i++) {
            player = Player[i];
            if(player.name == "None") {
                if(player.transformInto("Neon as Player 2"))
                    return;
                else
                    break;
            }
        }

        Console.print(this.__name + ": error");
    }

    fun wantNeon()
    {
        // Neon can be activated in-game or with a command-line option
        return Settings.wantNeonAsPlayer2 ||
               Application.args.hasOption("--want-neon-as-player-2");
    }
}