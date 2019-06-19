// -----------------------------------------------------------------------------
// File: collectible.ss
// Description: collectible script (pickup object)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Actor;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Video.Screen;
using SurgeEngine.Collisions.CollisionBall;
using SurgeEngine.Collisions.Sensor;

// a regular collectible that can be magnetized
object "Collectible" is "entity", "basic"
{
    transform = Transform();
    actor = Actor("Collectible");
    collider = CollisionBall(actor.height / 2);
    sfx = Sound("samples/collectible.wav");
    xsp = 0; ysp = 0; target = null;

    state "main"
    {
        // magnetism check
        for(i = 0; i < Player.count; i++) {
            if(Player[i].shield == "thunder") {
                dst = transform.position.distanceTo(Player[i].transform.position);
                if(dst <= 128)
                    magnetize(Player[i]);
            }
        }
    }

    state "disappearing"
    {
        if(actor.animation.finished)
            destroy();
    }

    state "magnetized"
    {
        // get delta
        dt = Time.delta;

        // compute speed
        ds = transform.position.directionTo(target.transform.position);
        sx = Math.sign(ds.x); sy = Math.sign(ds.y);
        xsp += ((sx == Math.sign(xsp)) ? 45 : -180) * sx * dt;
        ysp += ((sy == Math.sign(ysp)) ? 45 : -180) * sy * dt;

        // move towards the target
        transform.move(ds.x * xsp * dt, ds.y * ysp * dt);

        // demagnetize
        if(target.shield != "thunder")
            state = "main";
    }

    fun onCollision(otherCollider)
    {
        if(otherCollider.entity.hasTag("player"))
            pickup(otherCollider.entity);
    }

    fun pickup(player)
    {
        if(state != "disappearing") {
            player.collectibles++;
            sfx.play();
            actor.anim = 1;
            actor.zindex = 0.51;
            state = "disappearing";
        }
    }

    fun magnetize(player)
    {
        if(state == "main") {
            xsp = ysp = 0.0;
            target = player;
            state = "magnetized";
        }
    }
}

// this object is created whenever the player gets hit
// and its collectible counter is greater than zero
object "Scattered Collectible" is "entity", "disposable", "private"
{
    transform = Transform();
    actor = Actor("Collectible");
    collider = CollisionBall(actor.height / 2);
    sfx = Sound("samples/collectible.wav");
    vsensor = Sensor(0, -actor.height/2, 1, actor.height);
    hsensor = Sensor(-actor.width/2, 0, actor.width, 1);
    hlock = 0.0; vlock = 0.0;
    xsp = 0.0; ysp = 0.0;
    startTime = Time.time;

    state "main"
    {
        // delta
        dt = Time.delta;
        hlock -= dt;
        vlock -= dt;

        // horizontal movement
        if(hsensor.status != null) {
            if(hlock <= 0.0) {
                hlock = 0.25;
                xsp *= -0.25;
            }
        }

        // vertical movement
        if(vsensor.status != null) {
            if(vlock <= 0.0) {
                vlock = 0.25;
                if(ysp >= 0)
                    ysp *= -0.75;
                else
                    ysp *= -0.25;
            }
        }
        else
            ysp += 337.5 * dt; // gravity

        // move object
        transform.move(xsp * dt, ysp * dt);

        // timeout
        if(timeout(5.0))
            destroy();
    }

    state "disappearing"
    {
        if(actor.animation.finished)
            destroy();
    }

    fun onCollision(otherCollider)
    {
        if(Time.time >= startTime + 1.0) {
            if(otherCollider.entity.hasTag("player"))
                pickup(otherCollider.entity);
        }
    }

    fun pickup(player)
    {
        if(state != "disappearing") {
            player.collectibles++;
            sfx.play();
            actor.anim = 1;
            actor.zindex = 0.51;
            state = "disappearing";
        }
    }

    fun setVelocity(x, y)
    {
        xsp = x;
        ysp = y;
        return this;
    }
}

// Lucky Collectible moves in a spiral trajectory
// towards the player and is used to give a bonus
// HOWTO: Level.spawn("Lucky Collectible")
//          .setPlayer(player).setPhase(k++); // k integer
object "Lucky Collectible" is "private", "entity", "awake"
{
    radius = Screen.width * 0.6;
    transform = Transform();
    actor = Actor("Collectible");
    sfx = Sound("samples/collectible.wav");
    luckyPlayer = Player.active;
    phase = 0;
    t = 0;

    state "main"
    {
        // collision check
        if(t > 1) {
            pickup(luckyPlayer);
            return;
        }

        // spiral movement
        transform.position = Vector2(
            (radius * (1 - t)) * Math.cos(6.283 * t + phase),
            (radius * (1 - t)) * -Math.sin(6.283 * t + phase)
        ).plus(luckyPlayer.transform.position);
        t += Time.delta;
    }

    state "disappearing"
    {
        if(actor.animation.finished)
            destroy();
    }

    fun pickup(player)
    {
        if(state != "disappearing") {
            player.collectibles++;
            sfx.play();
            actor.anim = 1;
            actor.zindex = 0.51;
            state = "disappearing";
        }
    }

    // --- MODIFIERS ---

    // set the beneficiary
    fun setPlayer(player)
    {
        luckyPlayer = player;
        return this;
    }

    // set the spiral phase, k = 0, 1, 2, ...
    fun setPhase(k)
    {
        phase = Math.pi * k;
        return this;
    }
}

