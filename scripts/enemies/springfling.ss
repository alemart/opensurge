// -----------------------------------------------------------------------------
// File: springfling.ss
// Description: Springfling enemy script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Player;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;
using SurgeEngine.Behaviors.Enemy;
using SurgeEngine.Behaviors.Platformer;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Video.Screen;

// Springfling is a jumping baddie
object "Springfling" is "entity", "enemy"
{
    actor = Actor("Springfling");
    enemy = Enemy();
    platformer = Platformer();
    transform = Transform();
    sfx = Sound("samples/springfling.wav");

    state "main"
    {
        // setup
        platformer.speed = 32;
        platformer.jumpSpeed = 320;
        state = "stopped";
    }

    state "stopped"
    {
        actor.anim = 0;
        if(timeout(1.0) && !platformer.midair)
            state = "jumping";
    }

    state "jumping"
    {
        actor.anim = 1;
        if(actor.animation.finished) {
            sfx.volume = jumpVolume();
            sfx.play();
            platformer.walk();
            platformer.jump();
            state = "midair";
        }
    }

    state "midair"
    {
        if(timeout(0.25) && !platformer.midair) {
            platformer.stop();
            state = "landing";
        }
    }

    state "landing"
    {
        actor.anim = 2;
        if(actor.animation.finished)
            state = "stopped";
    }

    fun jumpVolume()
    {
        player = Player.active;
        vol = 1 - Math.abs(player.transform.position.x - transform.position.x) / Screen.width;
        return Math.max(0, vol);
    }
}