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
    clearedAnim = 2;

    state "main"
    {
        // the goal sign hasn't
        // been touched yet
    }

    state "rotating"
    {
        actor.anim = 1;
        if(actor.animation.finished) {
            actor.anim = clearedAnim;
            state = "clear";
        }
    }

    state "clear"
    {
        // the goal sign now
        // shows: level cleared!
    }

    fun onCollision(otherCollider)
    {
        if(state == "main" && otherCollider.entity.hasTag("player"))
            goal(otherCollider.entity);
    }

    fun goal(player)
    {
        if(!player.dying) {
            sfx.play();
            Level.clear();
            clearedAnim = pickAnim(player.name);
            state = "rotating";
        }
    }

    fun pickAnim(playerName)
    {
        if(playerName == "Surge")
            return 3;
        else if(playerName == "Neon")
            return 4;
        else if(playerName == "Charge")
            return 5;
        else
            return 2;
    }
}