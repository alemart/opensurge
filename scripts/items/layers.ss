// -----------------------------------------------------------------------------
// File: layers.ss
// Description: layer system script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Collisions.CollisionBox;

object "Layer Green" is "entity", "special"
{
    collider = CollisionBox(32, 32);

    state "main"
    {
    }

    fun onCollision(otherCollider)
    {
        if(otherCollider.entity.hasTag("player"))
            changeLayer(otherCollider.entity);
    }

    fun changeLayer(player)
    {
        player.layer = "green";
    }
}

object "Layer Yellow" is "entity", "special"
{
    collider = CollisionBox(32, 32);

    state "main"
    {
    }

    fun onCollision(otherCollider)
    {
        if(otherCollider.entity.hasTag("player"))
            changeLayer(otherCollider.entity);
    }

    fun changeLayer(player)
    {
        player.layer = "yellow";
    }
}