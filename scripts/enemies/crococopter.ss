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

// Crococopter has a helix that hits the player
object "Crococopter" is "entity", "enemy"
{
    actor = Actor("Crococopter");
    enemy = spawn("Enemy");
    helixCollider = CollisionBox(16, 8).setAnchor(0.5, 3);
    movement = spawn("DirectionalMovement");
    transform = Transform();
    t = 0;

    state "main"
    {
        /*
        // look at the player
        if(transform.position.x > Player.active.transform.position.x)
            actor.hflip = true;
        else
            actor.hflip = false;
        */

        // up-down movement
        movement.speed = 47 * Math.cos(3.1416 * t);
        t += Time.delta;

        if(timeout(5))
        transform.rotate(45 * Time.delta);
        //transform.lookAt(Player.active.transform);
        movement.angle = transform.angle;
        movement.speed = 15;
        movement.speed = 0;
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
        movement.direction = Vector2.up;
        //helixCollider.visible = true;
        //enemy.collider.visible = true;
    }
}