// -----------------------------------------------------------------------------
// File: tubes.ss
// Description: tube system
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;
using SurgeEngine.Player;
using SurgeEngine.Collisions.CollisionBox;

// Tube Out rolls and unlocks the player
object "Tube Out" is "entity", "special"
{
    public rollSpeed = 600;
    collider = CollisionBox(32, 32);
    player = null;

    state "main"
    {
    }

    state "roll"
    {
        player.roll();
        state = "main";
    }

    fun onCollision(otherCollider)
    {
        if(otherCollider.entity.hasTag("player")) {
            player = otherCollider.entity;
            player.input.enabled = true;
            if(shouldBoost(player))
                boost(player);
            state = "roll";
        }
    }

    fun boost(player)
    {
        if(player.speed >= 0)
            player.speed = Math.max(rollSpeed, player.speed);
        else
            player.speed = Math.min(-rollSpeed, player.speed);
    }

    fun shouldBoost(player)
    {
        return player.ysp >= 0;
    }
}

// Tube In locks the player
object "Tube In" is "entity", "special"
{
    collider = CollisionBox(32, 32);

    state "main"
    {
    }

    fun onCollision(otherCollider)
    {
        if(otherCollider.entity.hasTag("player")) {
            player = otherCollider.entity;
            player.input.enabled = false;
        }
    }
}