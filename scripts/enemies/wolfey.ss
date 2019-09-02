// -----------------------------------------------------------------------------
// File: wolfey.ss
// Description: Wolfey enemy script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Behaviors.Enemy;
using SurgeEngine.Behaviors.Platformer;

// Wolfey is a baddie that simply walks around
object "Wolfey" is "entity", "enemy"
{
    actor = Actor("Wolfey");
    enemy = Enemy();
    platformer = Platformer();

    state "main"
    {
        platformer.speed = 100;
        state = "walking";
    }

    state "walking"
    {
        if(timeout(3.0))
            state = "new direction";
    }

    state "new direction"
    {
        // turn around
        if(platformer.walkingRight)
            platformer.walkLeft();
        else
            platformer.walkRight();

        // go back
        state = "walking";
    }
}