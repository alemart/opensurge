// -----------------------------------------------------------------------------
// File: spikes.ss
// Description: spikes script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Vector2;
using SurgeEngine.Brick;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Collisions.CollisionBox;

object "Spikes" is "entity"
{
    actor = Actor("Spikes");
    brick = Brick("Spikes Mask");
    sfx = Sound("samples/spikes.wav");
    collider = CollisionBox(20, 8);

    state "main"
    {
    }

    fun constructor()
    {
        collider.setAnchor(0.5, actor.height / collider.height + 0.25);
        //collider.visible = true;
    }

    fun onOverlap(otherCollider)
    {
        if(otherCollider.entity.hasTag("player"))
            hit(otherCollider.entity);
    }

    fun hit(player)
    {
        if(
            !player.blinking && player.activity != "dying" &&
            !player.invincible && player.activity != "gettinghit"
        ) {
            player.hit(actor);
            sfx.play();
        }
    }
}

object "Spikes Down" is "entity"
{
    actor = Actor("Spikes Down");
    brick = Brick("Spikes Down Mask");
    sfx = Sound("samples/spikes.wav");
    collider = CollisionBox(20, 8);

    state "main"
    {
    }

    fun constructor()
    {
        collider.setAnchor(0.5, -actor.height / collider.height + 0.75);
        //collider.visible = true;
    }

    fun onOverlap(otherCollider)
    {
        if(otherCollider.entity.hasTag("player"))
            hit(otherCollider.entity);
    }

    fun hit(player)
    {
        if(
            !player.blinking && player.activity != "dying" &&
            !player.invincible && player.activity != "gettinghit"
        ) {
            player.hit(actor);
            sfx.play();
        }
    }
}