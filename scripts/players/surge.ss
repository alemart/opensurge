// -----------------------------------------------------------------------------
// File: surge.ss
// Description: companion objects for Surge
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Actor;
using SurgeEngine.Level;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Collisions.CollisionBall;
using SurgeEngine.Behaviors.DirectionalMovement;
using SurgeEngine.Vector2;

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
// Surge's sneakers won't be lit while he's midair
//
object "Surge's Light Sneakers" is "companion"
{
    player = Player("Surge");

    state "main"
    {
        if(player.midair) {
            if(player.walking) {
                if(player.anim == 1 || player.anim == 20)
                    setAnim(20, player.animation.speedFactor);
            }
            else if(player.running) {
                if(player.anim == 2 || player.anim == 21)
                    setAnim(21, player.animation.speedFactor);
            }
            else if(player.braking) {
                if(player.anim == 7 || player.anim == 22)
                    setAnim(22, player.animation.speedFactor);
            }
        }
    }

    fun setAnim(id, factor)
    {
        player.anim = id;
        player.animation.speedFactor = factor;
    }
}

//
// Surge's Shield Abilities performs an attack depending on the shield type
//
object "Surge's Shield Abilities" is "companion"
{
    player = Player("Surge");
    shieldAbilities = player.spawn("Shield Abilities").setup({
        "thunder": "Surge's Lighting Smash"
    });
}

//
// Surge's Lighting Smash (thunder shield)
//
object "Surge's Lighting Smash" is "shield ability"
{
    player = Player("Surge");
    sfx = Sound("samples/lighting_smash.wav");
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
            Level.spawnEntity("Surge's Lighting Spark", player.collider.center)
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
// Surge's Lighting Boom (double jump)
//
object "Surge's Lighting Boom" is "companion"
{
    player = Player("Surge");
    normalJumpSpeed = -240; // pixels per second
    superJumpSpeed = -330; // if thunder shield
    timeMidAir = 0;
    xsp = 0;
    fx = null;
    shieldAbilities = null;

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
        player.springify(); // restore sensors
        player.aggressive = true;
        xsp = player.xsp;
        fx = spawn("Surge's Lighting Boom FX");
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
}

//
// Special effects
//

// Lighting Boom Effect
object "Surge's Lighting Boom FX" is "private", "entity"
{
    actor = Actor("Surge's Lighting Boom");
    sfx = Sound("samples/lighting_boom.wav");
    collider = CollisionBall(40);
    player = Player("Surge");
    minRadius = 0;
    maxRadius = 0;
    time = 0;
    sparks = [];

    state "main"
    {
        // initializing
        minRadius = collider.radius;
        maxRadius = actor.width * 0.6;
        //collider.visible = true;
        sfx.play();

        // add sparks
        for(n = 16, i = 0; i < n; i++) {
            spark = spawn("Surge's Lighting Spark").setId(i, n);
            sparks.push(spark);
        }

        // configure the actor
        actor.visible = (player.shield == "thunder");
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
                enemy.kill(player);
            }
        }
    }
}

// Lighting Spark
object "Surge's Lighting Spark" is "private", "disposable", "entity"
{
    actor = Actor("Surge's Lighting Spark");
    movement = DirectionalMovement();
    id = 0;
    count = 0;
    baseAngle = null;

    state "main"
    {
        // initializing
        actor.zindex = 0.50005;
        movement.speed = 180;
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
}