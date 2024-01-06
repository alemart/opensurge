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
    leftShoulder = "fire5";
    rightShoulder = "fire6";
    secondaryActionButton = "fire2";

    state "main"
    {
        player = Player.active;
        input = player.input;

        // switch character by pressing L1 or R1
        if(input.buttonPressed(rightShoulder))
            switchCharacter(-1);
        else if(input.buttonPressed(leftShoulder))
            switchCharacter(+1);

        // We'll keep the following logic temporarily, for backwards
        // compatibility. Button "fire2" is the secondary action button,
        // and character switching now happens via shoulder buttons.
        // The secondary action button is currently unused elsewhere in
        // the game. We used it to do the character switching. This will
        // no longer be the case.

        // TODO: remove this fire2 trigger.
        else if(input.buttonPressed(secondaryActionButton))
            switchCharacter(+1);
    }

    fun switchCharacter(direction)
    {
        // is the character switching disabled?
        if(!enabled || Level.cleared || Player.count == 1)
            return;

        // switch character
        n = Player.count;
        for(i = 0; i < n; i++) {
            player = Player[i];
            if(player.hasFocus()) {

                if(!(player.midair || player.underwater || player.frozen)) {
                    nextPlayer = findNextFocusablePlayer(i, direction);
                    if(nextPlayer !== null) {
                        if(nextPlayer.focus())
                            adjustShoulderButtons(nextPlayer.input);
                    }
                }
                else
                    deny.play();

                break;

            }
        }
    }

    fun findNextFocusablePlayer(playerIndex, direction)
    {
        n = Player.count;
        i = playerIndex;

        for(k = 1; k < n; k++) {
            next = (i + (n + direction * k)) % n;
            player = Player[next];

            if(player.focusable)
                return player;
        }

        return null;
    }

    fun adjustShoulderButtons(input)
    {
        input.simulateButton(leftShoulder, true);
        input.simulateButton(rightShoulder, true);

        // TODO: remove this fire2 trigger.
        input.simulateButton(secondaryActionButton, true);
    }
}