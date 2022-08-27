// -----------------------------------------------------------------------------
// File: power_pluggy.ss
// Description: Power Pluggy gimmick
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Transform;
using SurgeEngine.Brick;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Player;
using SurgeEngine.Vector2;
using SurgeEngine.Collisions.CollisionBall;

object "Power Pluggy Clockwise" is "entity", "gimmick"
{
    brick = Brick("Power Pluggy Clockwise Mask");
    base = spawn("Power Pluggy Base");
    piOverTwo = Math.pi / 2;
    minGsp = 300;

    fun onPlayerEnter(player)
    {
        brick.enabled = true;

        if(player.gsp < minGsp) {
            player.gsp = minGsp;
        }
    }

    fun onPlayerExit(player)
    {
        brick.enabled = false;
    }

    fun isStartingToRotate(angle)
    {
        return angle < -piOverTwo;
    }

    fun onReset()
    {
        base.reset();
    }

    fun constructor()
    {
        brick.type = "solid";
        brick.offset = Vector2.zero;
        brick.enabled = false;
    }
}

object "Power Pluggy Counterclockwise" is "entity", "gimmick"
{
    brick = Brick("Power Pluggy Counterclockwise Mask");
    base = spawn("Power Pluggy Base");
    piOverTwo = Math.pi / 2;
    minGsp = 300;

    fun onPlayerEnter(player)
    {
        brick.enabled = true;

        if(player.gsp > -minGsp) {
            player.gsp = -minGsp;
        }
    }

    fun onPlayerExit(player)
    {
        brick.enabled = false;
    }

    fun isStartingToRotate(angle)
    {
        return angle > -piOverTwo && angle < 0;
    }

    fun onReset()
    {
        base.reset();
    }

    fun constructor()
    {
        brick.type = "solid";
        brick.offset = Vector2(-144+16, 0);
        brick.enabled = false;
    }
}

object "Power Pluggy Base" is "private", "entity", "awake"
{
    actor = Actor("Power Pluggy");
    sfxEnter = Sound("samples/power_pluggy_enter.wav");
    sfxExit = Sound("samples/power_pluggy_exit.wav");
    collider = spawn("Power Pluggy Collider").setOffset(actor.animation.actionOffset);
    transform = Transform();
    player = null;
    oldDot = 1.0;
    dot = 1.0;

    state "main"
    {
        // do nothing
    }

    state "done"
    {
        // do nothing
    }

    state "rotating"
    {
        // look at the player
        ds = player.transform.position.minus(transform.position);
        theta = Math.atan2(ds.y, ds.x);
        transform.angle = -Math.rad2deg(theta);
        //Console.print(Math.rad2deg(theta));

        // compute a dot product
        oldDot = dot;
        dot = ds.normalized().dot(Vector2.up);
        //dot = -ds.y / ds.length;
        //Console.print(dot);

        // we're just starting to rotate
        if(parent.isStartingToRotate(theta)) {
            transform.angle = 90;
            //Console.print("STARTING");
        }

        // let the player go
        else if(dot > oldDot) {
            if(timeout(0.25) || player.slope == 180) {
                sfxExit.play();
                player.input.enabled = true;
                parent.onPlayerExit(player);
                transform.angle = -90;
                player = null;
                state = "done";
                //Console.print("EXIT");
            }
        }
    }

    fun reset()
    {
        transform.angle = 90;
        player = null;
        state = "main";
    }

    fun onPlayerCollision(p)
    {
        //Console.print(p.name);
        if(state !== "main")
            return;

        player = p;
        dot = oldDot = 1.0;
        state = "rotating";

        sfxEnter.play();
        parent.onPlayerEnter(player);
        player.roll();
        player.input.enabled = false;

        //Console.print("ENTER");
    }

    fun constructor()
    {
        transform.angle = 90;
        actor.zindex = 0.51;
    }
}

object "Power Pluggy Collider" is "private", "entity", "awake"
{
    collider = CollisionBall(12);
    transform = Transform();

    fun setOffset(offset)
    {
        transform.localPosition = offset;
        return this;
    }

    fun onCollision(otherCollider)
    {
        if(otherCollider.entity.hasTag("player")) {
            player = otherCollider.entity;
            if(!player.midair && !player.frozen && !player.dying && !player.drowning) {
                parent.onPlayerCollision(player);
            }
        }
    }
}