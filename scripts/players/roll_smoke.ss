// -----------------------------------------------------------------------------
// File: roll_smoke.ss
// Description: roll smoke effect (companion object)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Actor;
using SurgeEngine.Level;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;

//
// Roll Smoke is a companion object that produces a
// neat smoke effect when the player is charging a dash
//
object "Roll Smoke"
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
        else if(timeout(0.35)) {
            spawnSmoke(0.75);
            state = "cool out";
        }
    }

    state "cool out"
    {
        state = "charging";
    }

    fun spawnSmoke(scale)
    {
        if(!player.midair) {
            Level.spawnEntity(
                "Roll Smoke Sprite",
                player.collider.center.translatedBy(player.direction * -22, dy - 3)
            ).setDirection(player.direction).setScale(scale);
        }
    }
}

object "Roll Smoke Sprite" is "entity", "private", "disposable"
{
    actor = Actor("Roll Smoke");
    transform = Transform();

    state "main"
    {
        if(actor.animation.finished)
            destroy();
    }

    fun setOffset(x, y)
    {
        actor.offset = Vector2(x, y);
        return this;
    }

    fun setDirection(direction)
    {
        actor.hflip = (direction < 0);
        return this;
    }

    fun setScale(scale)
    {
        transform.localScale = Vector2(scale, scale);
        return this;
    }
}