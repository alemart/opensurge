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
using SurgeEngine.Behaviors.Enemy;
using SurgeEngine.Behaviors.CircularMovement;
using SurgeEngine.Collisions.CollisionBox;

// Crococopter has a helix that hits the player
object "Crococopter" is "entity", "enemy"
{
    actor = Actor("Crococopter");
    enemy = Enemy();
    movement = CircularMovement();
    helixCollider = CollisionBox(16, 8).setAnchor(0.5, 3);
    transform = Transform();

    state "main"
    {
        // setup movement
        movement.radius = 24;
        movement.rate = 0.5;
        movement.scale = Vector2.up; // move only on the y-axis
        state = "active";
    }

    state "active"
    {
        // look at the player
        actor.hflip = (transform.position.x > Player.active.transform.position.x);
    }

    fun onCollision(otherCollider)
    {
        // player collided with the helix
        if(otherCollider.entity.hasTag("player")) {
            player = otherCollider.entity;
            player.getHit(actor);
        }
    }

    /*
    fun constructor()
    {
        // debug
        helixCollider.visible = true;
        enemy.collider.visible = true;
    }
    */
}