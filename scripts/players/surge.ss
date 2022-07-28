// -----------------------------------------------------------------------------
// File: surge.ss
// Description: companion objects for Surge
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Actor;
using SurgeEngine.Level;
using SurgeEngine.Vector2;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Collisions.Sensor;
using SurgeEngine.Collisions.CollisionBall;
using SurgeEngine.Behaviors.DirectionalMovement;

//
// Surge's waiting animation will be modified if he's underwater
//
object "Surge's Waiting Animation" is "companion"
{
    player = Player("Surge");

    state "main"
    {
        if(player.underwater && player.waiting)
            player.anim = 38;
    }
}

//
// We modify Surge's animation:
// 1. when he falls down after hitting a spring
// 2. after breathing an air bubble
//
object "Surge's Falling Animation" is "companion"
{
    player = Player("Surge");
    falling = 32;
    springing = 13;

    state "main"
    {
        // The player may be in the springing state,
        // but that alone doesn't mean he has just
        // been hit by a spring. We use the springing
        // state to create other things, like the double
        // jump, so we need to check player.anim as well.
        // We also check if regular physics are enabled.
        if(player.springing && player.anim == springing && !player.frozen)
            state = "watching";

        // From breathing to falling
        else if(player.breathing)
            state = "watching";
    }

    state "watching"
    {
        // Now the player is midair and will fall
        // down. We'll capture this event and
        // change the animation accordingly
        if(player.ysp >= 0 && player.midair && (player.springing || player.walking)) {
            player.anim = falling;
            state = "falling";
        }
        else if(!player.springing && !player.breathing)
            state = "main";
    }

    state "falling"
    {
        if(player.ysp >= 0 && player.midair)
            player.anim = falling;
        else
            state = "main";
    }
}

//
// Surge's Shield Abilities performs an attack depending on the shield type
//
object "Surge's Shield Abilities" is "companion"
{
    player = Player("Surge");
    shieldAbilities = player.spawn("Shield Abilities").setup({
        "thunder": "Surge's Lightning Smash"
    });
}

//
// Surge's Lightning Smash (thunder shield)
//
object "Surge's Lightning Smash" is "shield ability"
{
    player = Player("Surge");
    sfx = Sound("samples/lightning_smash.wav");
    abilities = null;

    state "main"
    {
    }

    state "active"
    {
        // back to ball mode
        player.anim = 41;
        if(player.animation.finished)
            state = "rolling";
        if(player.underwater)
            abilities.deactivate();
    }

    state "rolling"
    {
        player.roll();
        if(player.underwater)
            abilities.deactivate();
    }

    fun onActivate(shieldAbilities)
    {
        // smash!
        player.ysp = Math.max(player.ysp, 480);

        // create neat sparks
        for(n = 8, i = 0; i < n; i++) {
            Level.spawnEntity("Surge's Lightning Spark", player.collider.center)
                 .setDirection(Vector2.up)
                 .setId(i, n);
        }
        sfx.play();

        // save the shieldAbilities object
        abilities = shieldAbilities;

        // done
        state = "active";
    }

    fun onDeactivate(shieldAbilities)
    {
        state = "main";
    }
}

//
// Surge's Lightning Boom (double jump)
//
object "Surge's Lightning Boom" is "companion"
{
    player = Player("Surge");
    normalJumpSpeed = -240; // pixels per second
    superJumpSpeed = -330; // if thunder shield
    timeMidAir = 0;
    xsp = 0;
    fx = null;
    shieldAbilities = null;
    ceilingSensor = Sensor(-player.collider.width * 0.35, -player.collider.height * 0.75, player.collider.width * 0.7, 1);

    state "main"
    {
        // find the Shield Abilities object
        shieldAbilities = player.findObject("Shield Abilities") || 
                          player.spawn("Shield Abilities").setup({ });

        // done
        state = "watching";
    }

    state "watching"
    {
        timeMidAir += (player.midair ? Time.delta : -timeMidAir);
        if(timeMidAir >= 0.1 && !player.underwater) {
            if(player.input.buttonPressed("fire1") && isReady()) {
                boom();
                shieldAbilities.unlock();
                shieldAbilities.getReadyToActivate();
                return;
            }
        }

        if(!player.underwater)
            shieldAbilities.lock();
        else
            shieldAbilities.unlock();
    }

    state "attacking"
    {
        player.anim = 39;
        player.xsp = xsp;

        if(player.animation.finished) {
            player.springify(); // adjust sensors
            state = "sustaining";
        }
        else if(shieldAbilities.active) {
            state = "waiting for shield attack";
        }
        else if(mustStop()) {
            if(player.angle != 0)
                player.anim = 1;
            backToNormal();
        }
        else if(mustAdjustToCeiling())
            adjustToCeiling();
    }

    state "sustaining"
    {
        player.anim = 40;

        if(shieldAbilities.active) {
            state = "waiting for shield attack";
        }
        else if(mustStop()) {
            if(player.angle != 0)
                player.anim = 1;
            backToNormal();
        }
        else if(player.ysp >= 120) {
            backToNormal();
        }
        else if(mustAdjustToCeiling())
            adjustToCeiling();
    }

    state "waiting for shield attack"
    {
        if(!shieldAbilities.active)
            backToNormal();
    }

    fun boom()
    {
        player.hlock(0.5);
        player.ysp = jumpSpeed();
        player.aggressive = true;
        if(player.rolling)
            player.springify(); // adjust sensors
        xsp = player.xsp;
        fx = spawn("Surge's Lightning Boom FX");
        state = "attacking";
    }

    fun backToNormal()
    {
        if(state != "watching") {
            player.aggressive = false;
            state = "watching";
        }
    }

    fun isReady()
    {
        return player.midair && !shieldAbilities.active &&
               (player.jumping || player.rolling || player.springing);
    }

    fun mustStop()
    {
        return !player.midair || player.underwater ||
               player.hit || player.dying || player.frozen;
    }

    fun jumpSpeed()
    {
        jmp = (player.shield == "thunder") ? superJumpSpeed : normalJumpSpeed;
        return Math.min(player.ysp, jmp);
    }

    fun mustAdjustToCeiling()
    {
        return ceilingSensor.status == "solid";
    }

    fun adjustToCeiling()
    {
        player.ysp = Math.max(player.ysp, 0);
        for(maxAttempts = 64, i = 0; ceilingSensor.status == "solid" && i < maxAttempts; i++) {
            player.transform.translateBy(0, 1);
            ceilingSensor.onTransformChange();
        }
    }
}

//
// Special effects
//

// Lightning Boom Effect
object "Surge's Lightning Boom FX" is "private", "entity"
{
    actor = Actor("Surge's Lightning Boom");
    sfx = Sound("samples/lightning_boom.wav");
    collider = CollisionBall(40);
    player = Player("Surge");
    minRadius = 0;
    maxRadius = 0;
    time = 0;
    sparks = [];

    state "main"
    {
        stronger = (player.shield == "thunder");

        // initializing
        minRadius = collider.radius;
        maxRadius = actor.width * (stronger ? 0.6 : 0.4);
        //collider.visible = true;
        sfx.play();

        // add sparks
        for(n = 16, i = 0; i < n; i++) {
            spark = spawn("Surge's Lightning Spark")
                   .setSpeed(stronger ? 240 : 180)
                   .setId(i, n);
            sparks.push(spark);
        }

        // configure the actor
        actor.visible = stronger;
        actor.zindex = 0.50005;

        // done
        state = "expanding";
    }

    state "expanding"
    {
        // expand the collider radius
        time += Time.delta;
        timeToLive = actor.animation.frameCount / actor.animation.fps;
        collider.radius = Math.lerp(minRadius, maxRadius, time / timeToLive);

        // destroy the object
        if(time >= timeToLive)
            destroy();
    }

    fun onCollision(otherCollider)
    {
        // destroy enemies on touch
        if(otherCollider.entity.__name == "Enemy") {
            if(player.ysp <= 0) { // player going up?
                enemy = otherCollider.entity;
                if(enemy.enabled)
                    enemy.kill(player);
            }
        }
    }
}

// Lightning Spark
object "Surge's Lightning Spark" is "private", "disposable", "entity"
{
    actor = Actor("Surge's Lightning Spark");
    movement = DirectionalMovement();
    id = 0;
    count = 0;
    speed = 180;
    baseAngle = null;

    state "main"
    {
        // initializing
        actor.zindex = 0.50005;
        movement.speed = speed;
        if(baseAngle === null)
            movement.angle = 360 * id / count; // all directions
        else
            movement.angle = 180 * id / (count - 1) + (baseAngle - 90); // specific direction

        // done
        state = "moving";
    }

    state "moving"
    {
        if(actor.animation.finished)
            destroy();
    }

    fun setId(sparkId, sparkCount)
    {
        count = Math.max(2, sparkCount);
        id = Math.clamp(sparkId, 0, count - 1);
        return this;
    }

    fun setDirection(direction)
    {
        baseAngle = direction !== null ? direction.angle : null;
        return this;
    }

    fun setSpeed(spd)
    {
        speed = spd;
        return this;
    }
}
