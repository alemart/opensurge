// -----------------------------------------------------------------------------
// File: collectibles_listener.ss
// Description: triggers an event every time the collectibles counter reaches
//              certain multiples of a threshold (e.g., give the player one
//              extra life if he picks 100, 200, 300... collectibles)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Level;
using SurgeEngine.Events.Event;

//
// To give 1 extra life every 100 collectibles, you may call:
// collectiblesListener = spawn("Collectibles Listener").triggers("Give Extra Life").every(100);
//
object "Collectibles Listener"
{
    event = Event();
    counter = 0;
    divisor = 1;

    state "main"
    {
        // initialize the counter
        // Player.active.collectibles may or may not be zero at this moment
        counter = Math.floor(Player.active.collectibles / divisor);
        state = "listening";
    }

    state "listening"
    {
        c = Math.floor(Player.active.collectibles / divisor);
        if(c > counter) {
            counter = c;
            event.call();
        }
    }

    fun triggers(callback)
    {
        if(typeof callback == "string")
            event = spawn(callback);
        else if(typeof callback == "object")
            event = callback;

        return this;
    }

    fun every(numberOfCollectibles)
    {
        divisor = Math.max(1, Math.floor(numberOfCollectibles));
        return this;
    }
}
