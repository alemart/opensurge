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
// Release up after half a second and you'll gain a nice boost!
//
object "Super Peel Out" is "companion"
{
    public anim = 2; // default animation: running
    speed = 720;     // dash speed, in pixels/second

    charge = Sound("samples/charge.wav");
    release = Sound("samples/release.wav");
    player = parent; // since this object is configured as a
                     // companion, parent is the reference
                     // to the correct Player object

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
        // disable the dash?
        if(player.midair || player.hit || player.dying || player.winning) {
            state = "main";
            return;
        }

        // change the animation
        player.restore(); // stop looking up
        player.anim = anim;

        // make the player stand still
        player.xsp = player.ysp = player.gsp = 0;
        player.input.simulateButton("right", false);
        player.input.simulateButton("left", false);
        player.input.simulateButton("down", false);

        // ready to go?
        if(player.input.buttonReleased("up")) {
            if(timeout(0.5)) {
                player.gsp = speed * player.direction; // dash!!!
                release.play();
                spawnSmoke();
            }
            state = "main";
        }
        else if(player.input.buttonPressed("fire1"))
            charge.play();
    }

    fun spawnSmoke()
    {
        Level.spawnEntity(
            "Speed Smoke",
            Vector2(
                player.collider.center.x - player.direction * 22,
                player.collider.bottom + 2
            )
        ).setDirection(player.direction);
    }
}