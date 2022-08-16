// -----------------------------------------------------------------------------
// File: conveyor_belt.ss
// Description: conveyor belt
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;
using SurgeEngine.Collisions.CollisionBox;

//
// Conveyor Belt
//
// Conveyor Belts are implement as bricks - usually cloud bricks. This script
// is used to move the character when it is standing on a conveyor belt. The
// different groups of conveyor belts (A, B...) behave the same way, but you
// may assign different speeds to each of them using a setup script.
//
// Properties:
// - speed: number. Movement speed, given in pixels per second. A positive
//   speed moves the player to the right. A negative speed moves the player
//   to the left.
// - fps: number. Frames per second of the animation of the bricks. Used to
//   synchronize the movement with the animation.
//

object "Conveyor Belt A" is "gimmick", "entity"
{
    controller = Level.child("Conveyor Belt Controller A") || Level.spawn("Conveyor Belt Controller A");
    roi = spawn("Conveyor Belt ROI").setController(controller);

    fun set_speed(speed)
    {
        controller.speed = speed;
    }

    fun get_speed()
    {
        return controller.speed;
    }

    fun set_fps(fps)
    {
        controller.fps = fps;
    }

    fun get_fps()
    {
        return controller.fps;
    }
}

object "Conveyor Belt B" is "gimmick", "entity"
{
    controller = Level.child("Conveyor Belt Controller B") || Level.spawn("Conveyor Belt Controller B");
    roi = spawn("Conveyor Belt ROI").setController(controller);

    fun set_speed(speed)
    {
        controller.speed = speed;
    }

    fun get_speed()
    {
        return controller.speed;
    }

    fun set_fps(fps)
    {
        controller.fps = fps;
    }

    fun get_fps()
    {
        return controller.fps;
    }
}





object "Conveyor Belt Controller A"
{
    delegate = spawn("Conveyor Belt Controller");

    fun set_speed(speed)
    {
        delegate.speed = speed;
    }

    fun get_speed()
    {
        return delegate.speed;
    }

    fun set_fps(fps)
    {
        delegate.fps = fps;
    }

    fun get_fps()
    {
        return delegate.fps;
    }

    fun register(player)
    {
        delegate.register(player);
    }
}

object "Conveyor Belt Controller B"
{
    delegate = spawn("Conveyor Belt Controller");

    fun set_speed(speed)
    {
        delegate.speed = speed;
    }

    fun get_speed()
    {
        return delegate.speed;
    }

    fun set_fps(fps)
    {
        delegate.fps = fps;
    }

    fun get_fps()
    {
        return delegate.fps;
    }

    fun register(player)
    {
        delegate.register(player);
    }
}

object "Conveyor Belt Controller"
{
    public speed = -120; // movement speed, given in pixels per second
    public fps = 30; // frames per second of the animation of the bricks
    players = []; // players on the conveyor belt
    gspThreshold = 1.0;
    currentFrame = -1;

    state "main"
    {
        // move the players
        for(i = players.length - 1; i >= 0; i--) {
            player = players[i];
            move(player);
        }

        players.clear();
    }

    fun move(player)
    {
        if(!player.midair && !player.frozen) {
            // move the player synchronously if stopped or if moving in the
            // direction of the conveyor belt. Otherwise, move continuously
            if(Math.abs(player.gsp) < gspThreshold || speed * player.gsp > 0)
                moveSynchronously(player);
            else
                moveContinuously(player);

            // throw off the player when reaching the ledge that is at the
            // side that the conveyor belt is moving to
            if(speed * player.gsp <= 0) {
                if(player.balancing && player.direction * speed > 0) {
                    throwOff(player);
                }
                else {
                    slope = player.slope;
                    if(speed < 0 && slope > 0 && slope < 90)
                        throwOff(player);
                    else if(speed > 0 && slope > 270)
                        throwOff(player);
                }
            }
        }
    }

    fun moveSynchronously(player)
    {
        // synchronize the movement of the player with the animation of the bricks
        oldFrame = currentFrame;
        currentFrame = Math.floor(Time.time * fps);// % frameCount;

        if(currentFrame != oldFrame) {
            dx = speed / fps;
            player.transform.translateBy(dx, 0);
        }
    }

    fun moveContinuously(player)
    {
        player.gsp += speed * Time.delta;

        /*
        // FIXME when using the logic below, there is a corner case in which the
        // player will get stuck if walking slowly against the direction of the
        // conveyor belt. A constant player.gsp will move the player in one
        // direction, but the logic below will move it in the opposite direction
        // by approximately the same amount.
        dx = speed * Time.delta;
        player.transform.translateBy(dx, 0);
        */
    }

    fun throwOff(player)
    {
        player.gsp = speed;
    }

    fun register(player)
    {
        if(players.indexOf(player) < 0)
            players.push(player);
    }
}





object "Conveyor Belt ROI" is "private", "gimmick", "entity" // ROI = Region of Interest
{
    collider = CollisionBox(32, 32);
    controller = null;

    fun setController(ctrl)
    {
        controller = ctrl;
        return this;
    }

    fun onOverlap(otherCollider)
    {
        if(otherCollider.entity.hasTag("player")) {
            player = otherCollider.entity;
            controller.register(player);
        }
    }
}