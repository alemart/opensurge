// -----------------------------------------------------------------------------
// File: mosquito.ss
// Description: Mosquito enemy script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Vector2;
using SurgeEngine.Behaviors.Enemy;
using SurgeEngine.Behaviors.DirectionalMovement;

// Mosquito is a flying baddie
object "Mosquito" is "entity", "enemy"
{
    public anim = 0;

    actor = Actor("Mosquito");
    enemy = Enemy();
    movement = DirectionalMovement();

    state "main"
    {
        actor.anim = anim;
        actor.hflip = true;
        movement.direction = Vector2.left;
        movement.speed = 60;
    }
}