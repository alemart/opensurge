// -----------------------------------------------------------------------------
// File: brake_smoke.ss
// Description: brake smoke effect (companion object)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Level;
using SurgeEngine.Vector2;

//
// Brake Smoke is a companion object that produces a
// neat smoke effect when the player is braking
//
object "Brake Smoke" is "companion"
{
    player = parent;

    state "main"
    {
        if(player.braking)
            state = "smoking";
    }

    state "smoking"
    {
        spawnSmoke(1.0);
        state = "braking";
    }

    state "braking"
    {
        if(!player.braking)
            state = "main";
        else if(timeout(0.07))
            state = "smoking";
    }

    fun spawnSmoke(scale)
    {
        if(!player.midair) {
            feet = player.collider.center.plus(
                Vector2(0, player.collider.height / 2).rotatedBy(player.angle)
            );
            Level.spawnEntity("Smoke", feet).setScale(scale);
        }
    }
}