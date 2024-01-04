// -----------------------------------------------------------------------------
// File: checkpoint.ss
// Description: checkpoint script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Transform;
using SurgeEngine.Level;
using SurgeEngine.Collisions.CollisionBox;

object "Checkpoint" is "entity", "basic"
{
    sfx = Sound("samples/checkpoint.wav");
    actor = Actor("Checkpoint");
    collider = CollisionBox(18, 65).setAnchor(0.5, 1.0);
    transform = Transform();

    state "main"
    {
        // the checkpoint is
        // not yet active
    }

    state "activating"
    {
        actor.anim = 1;
        if(actor.animation.finished) {
            actor.anim = 2;
            state = "active";
        }
    }

    state "active"
    {
        // the checkpoint is
        // now active!
    }

    fun onCollision(otherCollider)
    {
        if(state == "main" && otherCollider.entity.hasTag("player"))
            checkpoint(otherCollider.entity);
    }

    fun checkpoint(player)
    {
        if(!player.dying && !player.secondary) {
            sfx.play();
            Level.spawnpoint = transform.position.translatedBy(0, -collider.height/2);
            state = "activating";
        }
    }
}