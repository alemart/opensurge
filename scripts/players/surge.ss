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

    state "main"
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
        if(!player.midair) {
            if(player.angle != 0)
                player.anim = 1;
            state = "main";
        }
        else if(player.animation.finished)
            state = "sustaining";
    }

    state "sustaining"
    {
        player.anim = 40;
        if(player.ysp >= 120) {
            player.springify();
            state = "main";
        }
        else if(!player.midair) {
            if(player.angle != 0)
                player.anim = 1;
            state = "main";
        }
    }

    fun isReady(player)
    {
        return player.midair && (player.jumping || player.rolling || player.springing);
    }

    fun jumpSpeed(player)
    {
        jmp = (player.shield == "thunder") ? superJumpSpeed : normalJumpSpeed;
        return Math.min(player.ysp, jmp);
    }
}

// Lighting Boom Effect
object "Surge's Lighting Boom FX" is "private", "entity"
{
    actor = Actor("Surge's Lighting Boom");
    sfx = Sound("samples/lighting_boom.wav");
    collider = CollisionBall(1);
    player = Player("Surge");
    minRadius = 0;
    maxRadius = 0;
    time = 0;

    state "main"
    {
        // initializing
        minRadius = collider.radius;
        maxRadius = actor.width * 0.65;
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
