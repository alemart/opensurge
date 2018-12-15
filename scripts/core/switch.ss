// -----------------------------------------------------------------------------
// File: switch.ss
// Description: character switching, pause & quit
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Input;
using SurgeEngine.Level;
using SurgeEngine.Player;
using SurgeEngine.Audio.Sound;

object "SwitchController"
{
    public enabled = true; // enable character switching?
    input = Input(null);
    deny = Sound("samples/deny.wav");

    state "main"
    {
        if(input.buttonPressed("fire3"))
            Level.pause();
        else if(input.buttonPressed("fire4"))
            Level.quit();
        else if(input.buttonPressed("fire2"))
            switchCharacter();
    }

    fun switchCharacter()
    {
        n = Player.count;
        if(n == 1 || !enabled) return;
        for(i = 0; i < n; i++) {
            p = Player(i);
            if(p.hasFocus()) {
                if(p.midair || p.underwater || p.frozen)
                    deny.play();
                else
                    Player(Math.mod(i + 1, n)).focus();
                break;
            }
        }
    }
}