// -----------------------------------------------------------------------------
// File: roll.ss
// Description: script of the "Roll" special object (used to roll the player)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Collisions.CollisionBox;

object "Roll" is "entity", "special"
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
            if(!player.midair)
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
}