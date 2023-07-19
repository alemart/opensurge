// -----------------------------------------------------------------------------
// File: lucky_bonus.ss
// Description: Lucky Bonus gives the player a bunch of collectibles in a neat way
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Level;
using SurgeEngine.Vector2;

object "Lucky Bonus"
{
    public player = Player.active;
    public bonus = 50;
    counter = 0;

    state "main"
    {
        if(timeout(0.02)) {
            if(++counter <= bonus)
                state = "lucky";
            else
                state = "done";
        }
    }

    state "lucky"
    {
        Level.spawnEntity("Lucky Collectible", Vector2.zero).setPlayer(player).setPhase(counter);
        state = "main";
    }

    state "done"
    {
        destroy();
    }
}

