// -----------------------------------------------------------------------------
// File: peel_out.ss
// Description: a dash move that can be added to playable characters
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;
using SurgeEngine.Vector2;
using SurgeEngine.Audio.Sound;

//
// This is a dash move that should be configured as a
// companion object in a character definition file (.chr)
//
// When you are stopped, hold up and press jump to charge.
// Release up after 0.3 second and you'll gain a nice boost!
//
object "Super Peel Out"
{
    charge = Sound("samples/charge.wav");
    release = Sound("samples/release.wav");
    player = parent; // since this object is configured as a
                     // companion, parent is the reference
                     // to the correct Player object

    speed = 720;     // dash speed, in pixels/second

    // capture the event
    state "main"
    {
        if(player.lookingUp) {
            if(player.input.buttonPressed("fire1")) {
                charge.play();
                state = "charging";
            }
        }
    }

    // charging the dash
    state "charging"
    {
        player.anim = 2; // running animation
        player.animation.speedFactor = 1.85;
        player.frozen = true; // disable physics (temporarily)

        // ready to go?
        if(player.input.buttonReleased("up")) {
            if(timeout(0.3)) {
                player.gsp = speed * player.direction; // dash!!!
                release.play();
                spawnSmoke();
            }
            player.frozen = false; // enable physics
            state = "main";
        }
        else if(player.input.buttonPressed("fire1"))
            charge.play();
    }

    fun spawnSmoke()
    {
        if(!player.midair) {
            Level.spawnEntity(
                "Speed Smoke",
                Vector2(
                    player.collider.center.x - player.direction * 22,
                    player.collider.bottom - 3
                )
            ).setDirection(player.direction);
        }
    }
}
