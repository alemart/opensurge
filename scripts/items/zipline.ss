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

    actor = Actor("Zipline Grabber");
    collider = CollisionBox(8, 1.5 * actor.height).setAnchor(
        actor.animation.hotspot.x / actor.width,
        actor.animation.hotspot.y / (1.5 * actor.height)
    );
    transform = Transform();
    grab = Sound("samples/zipline.wav");
    zipSound = spawn("Zipline Sound");
    speed = 0.0; // current speed in px/s
    maxSpeed = 640.0; // max speed in px/s
    zipline = null; // current zipline
    lockedPlayer = null;
    zippingPlayerHands = Vector2(-0.35, -0.8); // relative position of the hands
    zippingPlayerAnimation = 16; // player animation for "Zipping"
    direction = 1;

    state "main"
    {
        // setup
        actor.anim = anim;
        actor.zindex = 0.5;
        changeZiplinesAnim(anim);
        state = "idle";
    }

    state "idle"
    {
    }

    state "zipping"
    {
        if(zipline != null) {
            // move
            dt = Time.delta;
            acc = Level.gravity * zipline.direction.y;
            speed = Math.min(speed + acc * dt, maxSpeed);
            transform.move(zipline.direction.x * speed * dt, zipline.direction.y * speed * dt);

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
        transform.move(0, speed * Time.delta);
    }

    fun startZipping(player)
    {
        if(state == "idle") {
            lockPlayer(player);
            zipline = null;
            grab.play();
            zipSound.play();
            state = "zipping";
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
        player.transform.position = transform.position.translatedBy(
            -zippingPlayerHands.x * player.collider.width * player.direction,
            -zippingPlayerHands.y * player.collider.height
        );
    }

    fun changeZiplinesAnim(anim)
    {
        rzips = Level.children("Zipline Right");
        foreach(zipline in rzips)
            zipline.setAnim(anim);

        lzips = Level.children("Zipline Left");
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