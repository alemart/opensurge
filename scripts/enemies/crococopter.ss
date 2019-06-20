// -----------------------------------------------------------------------------
// File: crococopter.ss
// Description: Crococopter enemy script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Player;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;
using SurgeEngine.Collisions.CollisionBox;

// Crococopter has a helix that
// hits the player
object "Crococopter" is "entity", "enemy"
{
    actor = Actor("Crococopter");
    enemy = spawn("Enemy");
    transform = Transform();
    helixCollider = CollisionBox(16, 8).setAnchor(0.5, 3);
    t = 0;

    state "main"
    {
        // look at the player
        if(transform.position.x > Player.active.transform.position.x)
            actor.hflip = true;
        else
            actor.hflip = false;

        // up-down movement
        dt = Time.delta; t += dt;
        transform.move(0, 47 * Math.cos(3.1416 * t) * dt);
    }

    fun onCollision(otherCollider)
    {
        // player collided with the helix
        if(otherCollider.entity.hasTag("player")) {
            player = otherCollider.entity;
            player.hit(actor);
        }
    }

    fun constructor()
    {
        //helixCollider.visible = true;
        //enemy.collider.visible = true;
    }
}