// -----------------------------------------------------------------------------
// File: salamander_wall.ss
// Description: script for exploding walls
// Author: Cody Licorish
// License: MIT
// -----------------------------------------------------------------------------

using SurgeEngine.Actor;
using SurgeEngine.Brick;
using SurgeEngine.Collisions.CollisionBox;
using SurgeEngine.Level;
using SurgeEngine.Transform;

object "Salamander Exploding Wall" is "entity"
{
    public explosionTime = 3.0;
    actor = Actor("Salamander Exploding Wall");
    brick = Brick("Salamander Exploding Wall");
    collider = CollisionBox(actor.width, actor.height).setAnchor(0,0);
    transform = Transform();
    
    state "main"
    {
        // collider.visible = true;
    }
    state "exploding"
    {
        if(timeout(explosionTime)) {
            state = "inactive";
            actor.visible = false;
        }
    }
    state "inactive"
    {
        brick.enabled = false;
    }
    fun explode()
    {
        // explode
        state = "exploding";

        // spawn Explosion Combo
        diameter = actor.width;
        radius = diameter / 2;
        Level.spawnEntity(
            "Explosion Combo",
            transform.position.minus(actor.hotSpot).translatedBy(radius,radius)
        ).setSize(diameter, diameter).setDuration(explosionTime);
    }
}
