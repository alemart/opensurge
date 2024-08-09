// -----------------------------------------------------------------------------
// File: team_play.ss
// Description: Team Play is a mode of gameplay with AI and character switching
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Level;
using SurgeEngine.Video.Screen;

/*

Team Play is a mode of gameplay in which all playable characters in a level
have AI and in which character switching is enabled. The player may pick any
character at any time, and the others will follow.

How to use: add "Team Play" to the setup list of your .lev file.

*/

object "Team Play" is "setup"
{
    public enableWarnings = true;
    aiList = [];
    bgx = Level.child("Background Exchange Manager") ||
          Level.spawn("Background Exchange Manager");
    maxDistance = Screen.width / 2;

    state "main"
    {
        if(enableWarnings) {

            if(Player.count < 2)
                Console.print("Team Play requires at least two players in the level");

            if(Level.child("Surge Gameplay") === null)
                Console.print("Team Play may not work properly without Surge Gameplay");

        }

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

            if(aiList.indexOf(ai) < 0)
                aiList.push(ai);

            if(player.hasFocus() || !ai.enabled) {

                // the player is controlled by a human
                ai.focusable = true;

            }
            else {

                // the player is controlled by AI
                distance = player.transform.position.distanceTo(Player.active.transform.position);
                ai.focusable = (
                    !ai.repositioning && !player.frozen && distance < maxDistance &&
                    bgx.backgroundOfPlayer(player) === bgx.backgroundOfPlayer(Player.active)
                );

            }

            // check if the Player 2 companion is also present
            p2 = player.child("Player 2");
            if(p2 !== null)
                p2.focusable = ai.focusable;
        }
    }
}
