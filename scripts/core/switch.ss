// -----------------------------------------------------------------------------
// File: switch.ss
// Description: handles character switching
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;
using SurgeEngine.Player;
using SurgeEngine.Audio.Sound;

object "Switch Controller"
{
    public enabled = true; // enable character switching?
    deny = Sound("samples/deny.wav");
    secondaryActionButton = "fire2";

    state "main"
    {
        player = Player.active;
        input = player.input;

        if(input.buttonPressed(secondaryActionButton))
            switchCharacter(+1);
    }

    fun switchCharacter(direction)
    {
        // is the character switching disabled?
        if(!enabled || Level.cleared || Player.count == 1)
            return;

        // switch character
        player = Player.active;
        if(!player.frozen) {
            nextPlayer = findNextFocusablePlayer(player, direction);
            if(nextPlayer !== null) {
                if(nextPlayer.focus())
                    adjustSwitchButton(nextPlayer.input);
            }
        }
        else
            deny.play();
    }

    fun findNextFocusablePlayer(player, direction)
    {
        i = findPlayerIndex(player);
        n = Player.count;

        if(i < 0)
            return null;

        for(k = 1; k < n; k++) {
            next = (i + (n + direction * k)) % n;
            player = Player[next];

            if(player.focusable)
                return player;
        }

        return null;
    }

    fun findPlayerIndex(player)
    {
        n = Player.count;

        for(i = 0; i < n; i++) {
            if(Player[i] == player)
                return i;
        }

        return -1;
    }

    fun adjustSwitchButton(input)
    {
        input.simulateButton(secondaryActionButton, true);
    }
}