// -----------------------------------------------------------------------------
// File: zipline.ss
// Description: zipline script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Level;
using SurgeEngine.Player;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Collisions.CollisionBox;

object "Zipline Grabber" is "entity", "gimmick"
{
    public anim = 1;
    zippingPlayerAnimation = 16; // player animation for "Zipping"

    actor = Actor("Zipline Grabber");
    collider = CollisionBox(actor.width / 2, actor.height * 2).setAnchor(
        actor.animation.hotSpot.x / actor.width,
        actor.animation.hotSpot.y / (actor.height * 2)
    );
    transform = Transform();
    grab = Sound("samples/zipline.wav");
    zipSound = spawn("Zipline Sound");
    speed = 0.0; // current speed in px/s
    maxSpeed = 640.0; // max speed in px/s
    zipline = null; // current zipline
    lockedPlayer = null;
    direction = 1;

    state "main"
    {
        state = "idle";
    }

    state "idle"
    {
    }

    state "prepare to zip"
    {
        // we let onOverlap() find a zipline
        state = "zipping";
    }

    state "zipping"
    {
        if(zipline != null) {
            // move
            dt = Time.delta;
            acc = Level.gravity * zipline.direction.y;
            speed = Math.min(speed + acc * dt, maxSpeed);
            transform.translateBy(zipline.direction.x * speed * dt, zipline.direction.y * speed * dt);

            // store direction
            direction = Math.sign(zipline.direction.x);

            // update player
            lockedPlayer.frozen = true;
            lockedPlayer.anim = zippingPlayerAnimation;
            repositionPlayer(lockedPlayer);

            // finish prematurely
            if(lockedPlayer.hit || lockedPlayer.dying)
                finishZipping();
        }
        else {
            lockedPlayer.xsp = speed * direction;
            finishZipping();
        }

        zipline = null;
    }

    state "falling"
    {
        speed += Level.gravity * Time.delta;
        transform.translateBy(0, speed * Time.delta);
    }

    fun constructor()
    {
        actor.anim = anim;
        actor.zindex = 0.5;
        changeZiplinesAnim(anim);
    }

    fun startZipping(player)
    {
        if(state == "idle") {
            lockPlayer(player);
            grab.play();
            zipSound.play();
            zipline = null;
            state = "prepare to zip";
        }
    }

    fun finishZipping()
    {
        if(state == "zipping") {
            unlockPlayer();
            zipSound.stop();
            speed = 0;
            state = "falling";
        }
    }

    fun lockPlayer(player)
    {
        speed = Math.abs(player.speed);
        maxSpeed = Math.max(640, speed);
        player.gsp = player.xsp = player.ysp = 0;
        player.frozen = true;
        player.angle = 0;
        player.springify();
        lockedPlayer = player;
    }

    fun unlockPlayer()
    {
        if(lockedPlayer != null) {
            lockedPlayer.frozen = false;
            lockedPlayer.speed = speed * direction;
        }
        lockedPlayer = null;
    }

    fun repositionPlayer(player)
    {
        // if the action spot of the zipping animation of the player is
        // not defined*, use default values for the position of the hands
        if(player.actionSpot.x == 0 && player.actionSpot.y == 0) {
            player.transform.position = transform.position.translatedBy(
                Math.round(0.35 * player.collider.width) * player.direction,
                Math.round(0.80 * player.collider.height)
            );
            return;
        }

        // put the action spot of the zipping animation of the player at
        // the action spot of the zipline grabber
        player.transform.position = transform.position
                                    .plus(actor.actionOffset)
                                    .minus(player.actionOffset);
    }

    fun changeZiplinesAnim(anim)
    {
        rzips = Level.findEntities("Zipline Right");
        foreach(zipline in rzips)
            zipline.setAnim(anim);

        lzips = Level.findEntities("Zipline Left");
        foreach(zipline in lzips)
            zipline.setAnim(anim);
    }

    fun onCollision(otherCollider)
    {
        if(otherCollider.entity.hasTag("player")) {
            player = otherCollider.entity;
            if(!player.dying && !player.hit)
                startZipping(player);
        }
    }

    fun onOverlap(otherCollider)
    {
        entity = otherCollider.entity;
        if(entity.__name == "Zipline")
            zipline = entity;
    }

    fun onReset()
    {
        unlockPlayer();
        speed = 0;
        state = "idle";
    }
}

object "Zipline Right" is "entity", "gimmick"
{
    actor = Actor("Zipline Right");
    zipline = spawn("Zipline").setup(128, 64, 2, 1);

    state "main"
    {
        actor.zindex = 0.49;
        state = "done";
    }

    state "done"
    {
    }

    fun setAnim(anim)
    {
        actor.anim = anim;
        return this;
    }
}

object "Zipline Left" is "entity", "gimmick"
{
    actor = Actor("Zipline Left");
    zipline = spawn("Zipline").setup(128, 64, -2, 1);

    state "main"
    {
        actor.zindex = 0.49;
        state = "done";
    }

    state "done"
    {
    }

    fun setAnim(anim)
    {
        actor.anim = anim;
        return this;
    }
}

object "Zipline" is "private", "entity"
{
    public readonly direction = Vector2.zero;
    public readonly collider = null;

    fun setup(width, height, dx, dy)
    {
        collider = CollisionBox(width, height).setAnchor(0, 0);
        //collider.visible = true;
        direction = Vector2(dx, dy).normalized();
        return this;
    }
}

object "Zipline Sound"
{
    sfx = Sound("samples/zipline2.wav");
    volume = 0.0;

    state "main"
    {
    }

    state "playing"
    {
        sfx.volume = Math.min(1, volume);
        sfx.play();
        state = "cooldown";
    }

    state "cooldown"
    {
        volume += 0.25 * Time.delta;
        if(timeout(0.25))
            state = "playing";
    }

    fun play()
    {
        if(state == "main") {
            volume = 0.1;
            state = "playing";
        }
    }

    fun stop()
    {
        state = "main";
    }
}