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
using SurgeEngine.Transform;
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
            if(player.walking)
                setAnim(20, player.animation.speedFactor);
            else if(player.running)
                setAnim(21, player.animation.speedFactor);
            else if(player.braking)
                setAnim(22, player.animation.speedFactor);
        }
    }

    fun setAnim(id, f)
    {
        player.anim = id;
        player.animation.speedFactor = f;
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
    shieldAttack = null;

    state "main"
    {
        shieldAttack = player.findObject("Surge's Shield Attack");
        state = "watching";
    }

    state "watching"
    {
        timeMidAir += (player.midair ? Time.delta : -timeMidAir);
        if(timeMidAir >= 0.1 && !player.underwater) {
            if(isReady(player) && player.input.buttonPressed("fire1")) {
                player.hlock(0.5);
                player.ysp = jumpSpeed(player);
                xsp = player.xsp;
                fx = spawn("Surge's Lighting Boom FX");
                state = "attacking";
            }
        }
    }

    state "attacking"
    {
        player.anim = 39;
        player.xsp = xsp;

        if(player.animation.finished) {
            state = "sustaining";
        }
        else if(mustStop(player)) {
            if(player.angle != 0)
                player.anim = 1;
            state = "watching";
        }
        else if(player.input.buttonPressed("fire1")) {
            if(shieldAttack != null && shieldAttack.attack())
                state = "waiting for shield attack";
        }
    }

    state "sustaining"
    {
        player.anim = 40;

        if(player.ysp >= 120) {
            player.springify();
            state = "watching";
        }
        else if(mustStop(player)) {
            if(player.angle != 0)
                player.anim = 1;
            state = "watching";
        }
        else if(player.input.buttonPressed("fire1")) {
            if(shieldAttack != null && shieldAttack.attack())
                state = "waiting for shield attack";
        }
    }

    state "waiting for shield attack"
    {
        if(!shieldAttack.active)
            state = "watching";
    }

    fun isReady(player)
    {
        return player.midair &&
            (player.jumping || player.rolling || player.springing) &&
            (shieldAttack == null || !shieldAttack.active);
    }

    fun mustStop(player)
    {
        return !player.midair || player.underwater ||
            player.hit || player.dying || player.stopped || player.frozen;
    }

    fun jumpSpeed(player)
    {
        jmp = (player.shield == "thunder") ? superJumpSpeed : normalJumpSpeed;
        return Math.min(player.ysp, jmp);
    }
}

//
// Surge's Shield Attack performs an attack depending on the shield type
//
object "Surge's Shield Attack" is "companion"
{
    public readonly active = false;

    player = Player("Surge");
    lightingSmash = spawn("Surge's Lighting Smash");
    enabled = true;

    state "main"
    {
        if(!player.midair || player.springing || player.hit || player.dying || player.stopped || player.frozen)
            stop();
    }

    fun attack()
    {
        // select attack depending on the shield type
        if(enabled) {
            if(player.shield == "thunder")
                return perform(lightingSmash);
        }

        // no attack has been performed
        return false;
    }

    fun stop()
    {
        enabled = true;
        if(active) {
            active = false;
            player.aggressive = false;
        }
    }

    fun perform(specialMove)
    {
        active = true;
        player.aggressive = true;
        player.roll();

        // disable new shield attacks until
        // the player touches the ground
        enabled = false;

        // perform the attack
        specialMove.attack();

        // done
        return true;
    }
}

//
// Surge's Lighting Smash (thunder shield attack)
//
object "Surge's Lighting Smash"
{
    player = Player("Surge");
    sfx = Sound("samples/lighting_smash.wav");
    shieldAttack = parent;

    state "main"
    {
    }

    state "active"
    {
        player.anim = 41;
        if(player.underwater)
            shieldAttack.stop();
        if(!shieldAttack.active)
            state = "main";
    }

    fun attack()
    {
        sfx.play();
        player.ysp = Math.max(player.ysp, 480);
        for(n = 5, i = 0; i < n; i++)
            Level.spawnEntity("Surge's Lighting Spark", player.collider.center).setId(i, n);
        state = "active";
    }
}

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

    state "main"
    {
        // initializing
        minRadius = collider.radius;
        maxRadius = actor.width * 0.6;
        //collider.visible = true;
        actor.zindex = 0.50005;
        sfx.play();

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
        if(actor.animation.finished)
            destroy();
    }

    fun onCollision(otherCollider)
    {
        // destroy enemies on touch
        if(otherCollider.entity.__name == "Enemy") {
            enemy = otherCollider.entity;
            enemy.kill(player);
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

    state "main"
    {
        // initializing
        movement.speed = 180;
        movement.angle = 180 * id / (count - 1);
        actor.zindex = 0.50005;

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
}