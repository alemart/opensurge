// -----------------------------------------------------------------------------
// File: marmot.ss
// Description: Chain Marmot enemy script
// Author: Cody Licorish <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Behavior.Enemy;
using SurgeEngine.Behavior.Platformer;

// Green Marmot is a baddie that simply walks around
object "GreenMarmot" is "entity", "enemy"
{
    actor = Actor("GreenMarmot");
    enemy = Enemy();
    platformer = Platformer().walk();

    state "main"
    {
        platformer.speed = 60;
    }
}
