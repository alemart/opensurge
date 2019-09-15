// -----------------------------------------------------------------------------
// File: salamander.ss
// Description: Ruler salamander enemy script
// Author: Cody Licorish <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Behaviors.Enemy;
using SurgeEngine.Behaviors.Platformer;


// RulerSalamander is a baddie that simply walks around
object "RulerSalamander" is "entity", "enemy"
{
    actor = Actor("RulerSalamander");
    enemy = Enemy();
    platformer = Platformer().walk();
    public chargeRate = 0.2;
    charge_capacity = 0;
    next_direction = +1;

    state "main"
    {
        platformer.speed = 40;
        //platformer.speed = 20;
        if (charge_capacity < 1) {
          charge_capacity += chargeRate*Time.delta;
        } else {
          state = "relax";
        }
        setWalkingAnim();
    }
    state "relax"
    {
        if (timeout(5)) {
            charge_capacity = 0;
            state = "main";
        }
        setWalkingAnim();
    }

    fun setWalkingAnim() {
        anim = actor.animation;
        anim_number = anim.id;
        directionSwitch = false;
        // check charge capacity
        if (charge_capacity < 0.5) {
            anim_number = 0;
        } else if (charge_capacity < 1) {
            anim_number = 1;
        } else {
            anim_number = 2;
        }
        // check direction switch
        if (next_direction < 0) {
            if (platformer.leftWall || platformer.leftLedge) {
                next_direction = +1;
                directionSwitch = true;
            }
        } else {
            if (platformer.rightWall || platformer.rightLedge) {
                next_direction = -1;
                directionSwitch = true;
            }
        }
        if (next_direction < 0) {
            anim_number += 3;
        }
        // update, but maintain frame index
        if (anim_number != anim.id) {
            frame_number = anim.frame;
            actor.animation.id = anim_number;
            actor.animation.frame = frame_number;
        }
        return directionSwitch;
    }
}
