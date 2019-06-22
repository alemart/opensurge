// -----------------------------------------------------------------------------
// File: wolfey.ss
// Description: Wolfey enemy script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Behavior.Enemy;
using SurgeEngine.Behavior.Platformer;

// Wolfey is a baddie that simply walks around
object "Wolfey" is "entity", "enemy"
{
    actor = Actor("Wolfey");
    enemy = Enemy();
    platformer = Platformer().walk();

    state "main"
    {
        platformer.speed = 120;
    }
}