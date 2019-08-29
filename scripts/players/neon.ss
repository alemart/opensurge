// -----------------------------------------------------------------------------
// File: neon.ss
// Description: companion objects for Neon
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Level;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;
using SurgeEngine.Audio.Sound;

//
// Neon's Jetpack
//
object "Neon's Jetpack" is "companion"
{
    public readonly player = Player("Neon");
    soundhlp = spawn("Neon's Jetpack Sound Helper");
    smokehlp = spawn("Neon's Jetpack Smoke Helper");

    maxFlyingTime = 8; // seconds
    maxSpeed = 60; // px/s
    acceleration = 60; // px/s^2
    flyingTime = 0;

    state "main"
    {
        if(player.jumping && !player.underwater) {
            if(player.input.buttonReleased("fire1"))
                state = "ready";
        }
    }

    state "ready"
    {
        if(player.jumping && !player.underwater) {
            if(player.input.buttonPressed("fire1")) {
                player.ysp = Math.min(player.ysp, 60);
                flyingTime = 0;
                state = "flying";
            }
        }
        else
            state = "main";
    }

    state "flying"
    {
        if(canFly(player) && player.input.buttonDown("fire1")) {
            // fly
            player.anim = 20;
            player.ysp = Math.max(-maxSpeed, player.ysp - (Level.gravity + acceleration) * Time.delta);
            player.springify();

            // flying time
            flyingTime += Time.delta;
            if(flyingTime >= maxFlyingTime)
                state = "falling";
        }
        else
            state = "main";
    }

    state "falling"
    {
        if(canFly(player)) {
            player.anim = 21;
            player.ysp = Math.min(player.ysp, maxSpeed * 1.5);
        }
        else
            state = "main";
    }

    fun canFly(player)
    {
        return player.midair && !player.hit && !player.dying && !player.underwater && !player.winning;
    }

    fun get_flying()
    {
        return (state == "flying");
    }

    fun get_falling()
    {
        return (state == "falling");
    }
}

// This will play the jetpack sound
object "Neon's Jetpack Sound Helper"
{
    jetpack = parent;
    flying = Sound("samples/jetpack.wav");
    falling = Sound("samples/jetpack2.wav");

    state "main"
    {
        if(jetpack.flying) {
            flying.play();
            state = "wait";
        }
        else if(jetpack.falling) {
            falling.play();
            state = "wait";
        }
    }

    state "wait"
    {
        if(!jetpack.flying && !jetpack.falling) {
            flying.stop();
            falling.stop();
            state = "main";
        }
        else if(
            (jetpack.flying && timeout(1.0)) ||
            (jetpack.falling && timeout(0.3))
        ) {
            state = "main";
        }
    }
}

// This will produce the smoke when flying
object "Neon's Jetpack Smoke Helper"
{
    jetpack = parent;
    player = jetpack.player;
    baseSpeed = 30;

    state "main"
    {
        if(jetpack.flying) {
            Level.spawnEntity("Mini Smoke", player.collider.center.translatedBy(
                player.direction * -8, 8
            )).setVelocity(Vector2((Math.random() - 0.5) * baseSpeed, 3 * baseSpeed));
            state = "wait";
        }
    }

    state "wait"
    {
        if(timeout(0.03))
            state = "main";
    }
}
