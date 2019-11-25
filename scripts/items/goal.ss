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
            actor.anim = clearedAnim;
            state = "cleared";
        }
    }

    state "cleared"
    {
        // the goal sign now
        // shows: level cleared!
    }

    // given a player name, get the corresponding
    // animation ID of the "Goal" sprite
    fun animId(playerName)
    {
        if(playerName == "Surge")
            return 3;
        else if(playerName == "Neon")
            return 4;
        else if(playerName == "Charge")
            return 5;
        else
            return 6; // generic "cleared" animation
    }

    fun goal(player)
    {
        if(!player.dying) {
            sfx.play();
            Level.clear();
            clearedAnim = animId(player.name);
            state = "rotating";
        }
    }

    fun onCollision(otherCollider)
    {
        if(state == "not cleared" && otherCollider.entity.hasTag("player"))
            goal(otherCollider.entity);
    }
}