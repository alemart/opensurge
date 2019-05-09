// -----------------------------------------------------------------------------
// File: spring_booster.ss
// Description: spring booster script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Brick;
using SurgeEngine.Player;
using SurgeEngine.Level;
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Collisions.CollisionBox;

object "Spring Booster Right" is "entity", "gimmick"
{
    actor = Actor("Spring Booster Right");
    brick = Brick("Spring Booster Block");
    sensorCollider = CollisionBox(96, 32);
    springCollider = CollisionBox(16, 32);

    state "main"
    {
    }

    fun constructor()
    {
        brick.type = "cloud";
        sensorCollider.setAnchor(0.5, 1);
        springCollider.setAnchor(0, 1);
        sensorCollider.visible = true;
        springCollider.visible = true;
    }

    fun onReset()
    {
        
    }
}