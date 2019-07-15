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

    state "main"
    {
        if(!Level.cleared) {
            if(Player.active.input.buttonPressed("fire2"))
                switchCharacter();
        }
    }

    fun switchCharacter()
    {
        n = Player.count;
        if(n == 1 || !enabled)
            return;
        for(i = 0; i < n; i++) {
            p = Player[i];
            if(p.hasFocus()) {
                if(p.midair || p.underwater || p.frozen)
                    deny.play();
                else
                    Player[(i + 1) % n].focus();
                break;
            }
        }
    }
}