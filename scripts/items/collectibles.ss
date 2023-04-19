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

// Base code for a Collectible (used by the variations of a collectible)
// Call base.pickup(player) whenever the collectible is picked up
// The Base Collectible is not supposed to be used directly as a level object;
// see the Collectible object instead for an example of how to use it.
object "Base Collectible" is "private", "entity"
{
    actor = Actor("Collectible");
    sfx = Sound("samples/collectible.wav");
    value = 1;

    state "main"
    {
    }

    state "disappearing"
    {
        if(actor.animation.finished)
            parent.destroy();
    }

    fun pickup(player)
    {
        if(state != "disappearing" && !player.dying) {
            player.collectibles += value;
            sfx.play();
            actor.anim = 1;
            actor.zindex = 0.5001;
            state = "disappearing";
        }
    }

    fun get_radius()
    {
        return actor.height / 2;
    }

    fun get_disappearing()
    {
        return state == "disappearing";
    }

    fun get_zindex()
    {
        return actor.zindex;
    }

    fun set_zindex(zindex)
    {
        actor.zindex = zindex;
    }

    fun constructor()
    {
        actor.animation.sync = true;
    }



    // --- MODIFIERS ---

    // whenever the player picks up this collectible, what is the
    // number that should be added to the collectible counter? (default: 1)
    fun setValue(newValue)
    {
        value = newValue;
        return this;
    }
}

// a regular collectible that can be magnetized
object "Collectible" is "entity", "basic"
{
    base = spawn("Base Collectible");
    collider = CollisionBall(base.radius);
    transform = Transform();
    xsp = 0; ysp = 0;
    target = null; // target player (when magnetized)

    state "main"
    {
        // magnetism check
        for(i = Player.count - 1; i >= 0; i--) {
            if(shouldMagnetize(Player[i]))
                magnetize(Player[i]);
        }
    }

    state "magnetized"
    {
        // get delta
        dt = Time.delta;

        // compute speed
        ds = transform.position.directionTo(target.transform.position);
        sx = Math.sign(ds.x);
        sy = Math.sign(ds.y);
        xsp += 600 * ((sx == Math.sign(xsp)) ? 1 : -4) * sx * dt;
        ysp += 600 * ((sy == Math.sign(ysp)) ? 1 : -4) * sy * dt;

        // move towards the target
        transform.translateBy(ds.x * xsp * dt, ds.y * ysp * dt);

        // demagnetize
        if(base.disappearing || target.shield != "thunder")
            state = "main";
    }

    fun onCollision(otherCollider)
    {
        if(otherCollider.entity.hasTag("player")) {
            player = otherCollider.entity;
            base.pickup(player);
        }
    }

    fun shouldMagnetize(player)
    {
        return (player.shield == "thunder") &&
               (!base.disappearing) &&
               (transform.position.distanceTo(player.transform.position) <= 160);
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
object "Bouncing Collectible" is "entity", "disposable", "private"
{
    base = spawn("Base Collectible");
    transform = Transform();
    collider = CollisionBall(base.radius);
    vsensor = Sensor(0, -base.radius, 1, 2 * base.radius);
    hsensor = Sensor(-base.radius, 0, 2 * base.radius, 1);
    hlock = 0.0; vlock = 0.0;
    xsp = 0.0; ysp = 0.0;
    startTime = Time.time;
    timeToLive = 5.0; // in seconds

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
        transform.translateBy(xsp * dt, ysp * dt);
        
        // timeout
        if(timeout(timeToLive))
            destroy();
    }

    state "stopped"
    {
    }

    fun onCollision(otherCollider)
    {
        if(Time.time >= startTime + 1.0) {
            if(otherCollider.entity.hasTag("player")) {
                player = otherCollider.entity;
                base.pickup(player);
                state = "stopped";
            }
        }
    }



    // --- MODIFIERS ---

    // set the velocity of this object, in px/s
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
//          .setPlayer(player).setPhase(k++); // where k = 0, 1, 2...
object "Lucky Collectible" is "private", "entity", "awake"
{
    base = spawn("Base Collectible");
    transform = Transform();
    radius = Math.max(Screen.width, Screen.height) * 0.6;
    luckyPlayer = Player.active;
    phase = 0;
    t = 0;

    state "main"
    {
        // group these lucky collectibles in the render queue
        base.zindex = 1.00123;

        // setup spiral movement
        playerPos = luckyPlayer.transform.position;
        transform.position = Vector2(
            radius * -Math.cos(phase) + playerPos.x,
            radius * -Math.sin(phase) + playerPos.y
        );

        // start movement
        state = "moving";
    }

    state "moving"
    {
        // advance the timer
        t += Time.delta;

        // collision check
        if(t >= 1) {
            base.pickup(luckyPlayer);
            state = "done";
        }

        // stop if player is killed
        else if(luckyPlayer.dying) {
            state = "done";
        }
    }

    state "done"
    {
    }


    fun move(prevPos, playerPos)
    {
        // spiral movement
        r = radius * (1 - t);
        w = 6.2832 * t + phase;
        x = r * -Math.cos(w) + playerPos.x;
        y = r * -Math.sin(w) + playerPos.y;

        dx = x - prevPos.x;
        dy = y - prevPos.y;
        transform.translateBy(dx, dy);
    }

    fun lateUpdate()
    {
        if(state == "moving")
            move(transform.position, luckyPlayer.transform.position);
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

