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
    public readonly player = parent;

    soundHelper = spawn("Neon's Jetpack Sound Helper");
    smokeHelper = spawn("Neon's Jetpack Smoke Helper");
    repositioningMethodSetup = spawn("Neon's Jetpack Repositioning Method Setup");

    flyingAnim = 20;
    fallingAnim = 21;
    maxFlyingTime = 8; // seconds
    maxSpeed = 60; // px/s
    acceleration = 60; // px/s^2

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
            player.anim = flyingAnim;
            player.ysp = Math.max(-maxSpeed, player.ysp - (Level.gravity + acceleration) * Time.delta);
            player.springify();

            // flying time
            if(timeout(maxFlyingTime))
                state = "falling";
        }
        else
            state = "main";
    }

    state "falling"
    {
        if(canFly(player)) {
            player.anim = fallingAnim;
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
        return (state == "flying") || repositioningMethodSetup.isRepositioning;
    }

    fun get_falling()
    {
        return (state == "falling") && !repositioningMethodSetup.isRepositioning;
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
            direction = player.direction * (player.hflip ? -1 : 1);
            xvel = (Math.random() - 0.5) * baseSpeed;
            yvel = 3 * baseSpeed;

            Level.spawnEntity(
                "Mini Smoke",
                player.collider.center.translatedBy(direction * -8, 8)
            ).setVelocity(Vector2(xvel, yvel));

            state = "wait";
        }
    }

    state "wait"
    {
        if(timeout(0.03))
            state = "main";
    }
}




/*
 * The code below is related to a repositioning method that is used when Neon
 * is chosen as Player 2. It modifies the Flying Repositioning Method in ways
 * that are specific to Neon.
 *
 * The repositioning method is used, for example, when Neon is controlled by the
 * CPU. When he gets offscreen, he'll be repositioned (respawned) automatically.
 */

object "Neon's Jetpack Repositioning Method Setup"
{
    public isRepositioning = false; // will be true while Neon is being repositioned

    jetpack = parent;
    player = jetpack.player;

    state "main"
    {
        // Neon uses the Flying Repositioning Method instead of the default one.
        // We'll check if Neon has any of the following companions and, if he
        // does, we'll change the repositioning methods of each one of them.
        foreach(companionName in ["Player 2", "Follow the Leader AI"]) {
            companion = player.child(companionName);
            if(companion !== null) {
                companion.repositioningMethod = companion.spawn("Flying Repositioning Method");
                companion.repositioningMethod.flyingAnim = 20;
                companion.repositioningMethod.onStart = spawn("Neon's Jetpack Repositioning Method Start");
                companion.repositioningMethod.onFinish = spawn("Neon's Jetpack Repositioning Method Finish");
            }
        }

        // we're done with this setup
        state = "done";
    }

    state "done"
    {
    }
}

object "Neon's Jetpack Repositioning Method Start" is "event"
{
    repositioningMethodSetup = parent;

    fun call()
    {
        repositioningMethodSetup.isRepositioning = true;
    }
}

object "Neon's Jetpack Repositioning Method Finish" is "event"
{
    repositioningMethodSetup = parent;

    fun call()
    {
        repositioningMethodSetup.isRepositioning = false;
    }
}