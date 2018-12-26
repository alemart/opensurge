// -----------------------------------------------------------------------------
// File: collectible.ss
// Description: collectible script (pickup object)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Actor;
using SurgeEngine.Transform;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Collisions.CollisionBall;

object "Collectible" is "entity"
{
    transform = Transform();
    collider = CollisionBall(8);
    actor = Actor("Collectible");
    sfx = Sound("sfx/collectible.wav");
    xsp = 0; ysp = 0; target = null;

    state "main"
    {
        for(i = 0; i < Player.count; i++) {
            if(Player(i).shield == "thunder") {
                dst = transform.position.distanceTo(Player(i).transform.position);
                if(dst <= 64)
                    magnetize(Player(i));
            }
        }
    }

    state "disappearing"
    {
        if(actor.animation.finished)
            destroy();
    }

    state "magnetised"
    {

        // demagnetize
        if(target.shield != "thunder")
            state = "main";
    }

    fun onCollision(otherCollider)
    {
        if(otherCollider.entity.hasTag("player"))
            pickup(otherCollider.entity);
    }

    fun pickup(player)
    {
        if(state != "disappearing") {
            player.collectibles++;
            sfx.stop(); sfx.play();
            actor.anim = 1;
            state = "disappearing";
        }
    }

    fun magnetize(player)
    {
        if(state == "main") {
            target = player;
            state = "magnetised";
        }
    }
}