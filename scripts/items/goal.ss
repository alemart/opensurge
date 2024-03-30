// -----------------------------------------------------------------------------
// File: goal.ss
// Description: goal sign script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Transform;
using SurgeEngine.Level;
using SurgeEngine.Collisions.CollisionBox;

object "Goal" is "entity", "basic"
{
    sfx = Sound("samples/endsign.wav");
    actor = Actor("Goal");
    collider = CollisionBox(actor.width, actor.height).setAnchor(0.5, 1.0);
    transform = Transform();
    clearedPlayer = "";

    state "main"
    {
        // setup
        actor.anim = 1;
        state = "not cleared";
    }

    state "not cleared"
    {
        // the goal sign hasn't
        // been touched yet
    }

    state "rotating"
    {
        actor.anim = 2;
        if(actor.animation.finished) {
            updateSprite(clearedPlayer);
            state = "cleared";
        }
    }

    state "cleared"
    {
        // the goal sign now
        // shows: level cleared!
    }

    fun updateSprite(playerName)
    {
        // check if a character-specific goal sign exists
        newActor = Actor("Goal " + playerName);
        if(!newActor.animation.exists) {
            newActor.destroy();
            actor.anim = 3; // "cleared"
            return;
        }

        // if it does, replace it
        actor.destroy();
        actor = newActor;
    }

    fun goal(player)
    {
        sfx.play();
        Level.clear();
        clearedPlayer = player.name;
        state = "rotating";
    }

    fun accept(player)
    {
        return !player.dying && !player.secondary;
    }

    fun onCollision(otherCollider)
    {
        if(state == "not cleared" && otherCollider.entity.hasTag("player")) {
            player = otherCollider.entity;
            if(accept(player))
                goal(player);
        }
    }
}