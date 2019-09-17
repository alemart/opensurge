// -----------------------------------------------------------------------------
// File: explosion_combo.ss
// Description: explosion combo script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Level;

//
// An Explosion Combo is a set of explosions that
// occur within a certain area, during some time
//
// Functions:
//
// - setSize(width, height):  Sets the size of the area of the explosion combo.
//                            The area is centered on the position of this object.
//                            Returns this.
//
// - setDuration(seconds):    Sets the duration of the explosion combo.
//                            Returns this.
//
object "Explosion Combo" is "private", "entity"
{
    transform = Transform();
    width = 64; // width of the explosion area, in pixels
    height = 64; // height of the explosion area, in pixels
    explosionCount = 16; // explode no more than explosionCount times
    explosionTime = 0.125; // seconds
    duration = 1.0; // seconds
    timer = 0.0;
    
    state "main"
    {
        // explosion timers
        if(!timeout(duration)) {
            timer += Time.delta;
            if(timer >= explosionTime) {
                explode();
                timer -= explosionTime;
            }
        }
        else
            destroy();
    }

    fun explode()
    {
        // compute the explosion offset
        lenx = Math.floor(width / 8);
        leny = Math.floor(height / 4);
        gridx = Math.floor(Math.random() * (1 + lenx)) - lenx / 2;
        gridy = Math.floor(Math.random() * (1 + leny)) - leny / 2;

        // create explosion
        Level.spawnEntity("Explosion",
            transform.position.translatedBy(
                8 * gridx,
                4 * gridy
            )
        );
    }

    // Define the size of the area
    fun setSize(w, h)
    {
        width = Math.max(w, 0);
        height = Math.max(h, 0);
        return this;
    }

    // Define the duration of the explosion combo
    fun setDuration(seconds)
    {
        duration = Math.max(seconds, 0);
        return this;
    }
}