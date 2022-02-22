// -----------------------------------------------------------------------------
// File: dash_smoke.ss
// Description: dash smoke effect (companion object)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Actor;
using SurgeEngine.Level;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;

//
// Dash Smoke is a companion object that produces a
// neat smoke effect when the player is charging a dash
//
object "Dash Smoke" is "companion"
{
    player = parent;
    dy = 0;

    state "main"
    {
        if(player.charging) {
            dy = player.collider.bottom - player.collider.center.y;
            spawnSmoke(0.75);
            state = "charging";
        }
    }

    state "charging"
    {
        if(!player.charging) {
            spawnSmoke(1.0);
            state = "main";
        }
        else if(timeout(0.25)) {
            spawnSmoke(0.75);
            state = "cool out";
        }
    }

    state "cool out"
    {
        if(!player.midair)
            state = "charging";
        else
            state = "main";
    }

    fun spawnSmoke(scale)
    {
        if(!player.midair) {
            Level.spawnEntity(
                "Speed Smoke",
                player.collider.center.translatedBy(player.direction * -22, dy - 3)
            ).setDirection(player.direction).setScale(scale);
        }
    }
}
