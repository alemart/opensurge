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

object "Spikes" is "entity", "basic"
{
    public periodic = false; // if set to true, the spikes will repeatedly appear and disappear

    actor = Actor("Spikes");
    brick = Brick("Spikes Mask");
    collider = CollisionBox(24, 8);
    sfx = Sound("samples/spikes.wav");
    appearSfx = Sound("samples/spikes_appearing.wav");
    disappearSfx = Sound("samples/spikes_disappearing.wav");
    period = 8.0; // period of one appear-disappear cycle, in seconds

    state "main"
    {
        if(periodic)
            state = "active";
    }

    state "active"
    {
        actor.visible = true;
        brick.enabled = true;
        collider.enabled = true;
        if(Time.time % period >= period / 2) {
            disappearSfx.play();
            state = "inactive";
        }
    }

    state "inactive"
    {
        actor.visible = false;
        brick.enabled = false;
        collider.enabled = false;
        if(Time.time % period < period / 2) {
            appearSfx.play();
            state = "active";
        }
    }

    fun constructor()
    {
        collider.setAnchor(0.5, actor.height / collider.height + 0.25);
        //collider.visible = true;
    }

    fun onOverlap(otherCollider)
    {
        if(otherCollider.entity.hasTag("player")) {
            player = otherCollider.entity;
            hit(player);
        }
    }

    fun hit(player)
    {
        if(
            !player.blinking && !player.dying &&
            !player.invincible && !player.hit
        ) {
            // do not hit if the player is midair
            if(player.midair || player.ysp < 0)
                return;

            // adjust player position
            if(player.collider.bottom > collider.top) {
                dy = player.collider.bottom - player.transform.position.y;
                player.transform.translateBy(0, collider.top - dy - player.transform.position.y);
            }

            // hit player
            player.getHit(actor);
            sfx.play();
        }
    }
}

object "Spikes Down" is "entity", "basic"
{
    public periodic = false;

    actor = Actor("Spikes Down");
    brick = Brick("Spikes Down Mask");
    sfx = Sound("samples/spikes.wav");
    collider = CollisionBox(24, 8);
    appearSfx = Sound("samples/spikes_appearing.wav");
    disappearSfx = Sound("samples/spikes_disappearing.wav");
    period = 8.0; // period of one appear-disappear cycle, in seconds

    state "main"
    {
        if(periodic)
            state = "active";
    }

    state "active"
    {
        actor.visible = true;
        brick.enabled = true;
        collider.enabled = true;
        if(Time.time % period >= period / 2) {
            disappearSfx.play();
            state = "inactive";
        }
    }

    state "inactive"
    {
        actor.visible = false;
        brick.enabled = false;
        collider.enabled = false;
        if(Time.time % period < period / 2) {
            appearSfx.play();
            state = "active";
        }
    }

    fun constructor()
    {
        collider.setAnchor(0.5, -actor.height / collider.height + 0.75);
    }

    fun onOverlap(otherCollider)
    {
        if(otherCollider.entity.hasTag("player")) {
            player = otherCollider.entity;
            hit(player);
        }
    }

    fun hit(player)
    {
        if(
            !player.blinking && !player.dying &&
            !player.invincible && !player.hit
        ) {
            // adjust player position
            if(player.collider.top < collider.bottom) {
                dy = player.transform.position.y - player.collider.top;
                player.transform.translateBy(0, collider.bottom + dy - player.transform.position.y);
            }

            // hit player
            player.getHit(actor);
            sfx.play();
        }
    }
}
