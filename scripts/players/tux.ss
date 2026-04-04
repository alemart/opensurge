// -----------------------------------------------------------------------------
// File: tux.ss
// Description: companion objects for Tux
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Actor;
using SurgeEngine.Level;
using SurgeEngine.Vector2;

object "Tux's Running Animation" is "companion"
{
    player = parent;

    state "main"
    {
        if(player.underwater && player.running) {
            player.anim = 1;
            player.animation.speedFactor = 1.35;
        }
    }
}
