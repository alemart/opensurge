// -----------------------------------------------------------------------------
// File: team_play.ss
// Description: Team Play is a mode of gameplay with AI and character switching
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Level;

/*

Team Play is a mode of gameplay in which all playable characters in a level
have AI and in which character switching is enabled. The player may pick any
character at any time, and the others will follow.

How to use: add "Team Play" to the setup list of your .lev file.

*/

object "Team Play" is "setup"
{
    aiList = [];

    state "main"
    {
        if(Player.count < 2)
            Console.print("Team Play requires at least two players in the level");

        if(Level.child("Surge Gameplay") === null)
            Console.print("Team Play may not work properly without Surge Gameplay");

        state = "watch";
    }

    state "watch"
    {
        // Every player should have an AI companion. For each player, we check
        // if its AI companion is present in every frame, because players may
        // be transformed and their companion list may be reset.
        for(i = 0; i < Player.count; i++) {
            player = Player[i];

            ai = player.child("Follow the Leader AI") ||
                 player.spawn("Follow the Leader AI");

            if(aiList.indexOf(ai) < 0) {
                ai.focusable = true;
                aiList.push(ai);
            }
        }
    }
}
