// -----------------------------------------------------------------------------
// File: fish.ss
// Description: Fish baddie script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Level;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;
using SurgeEngine.Behaviors.Enemy;
using SurgeEngine.Behaviors.DirectionalMovement;
using SurgeEngine.Collisions.Sensor;

// Fish is a baddie that moves left and right
// It only appears if underwater
object "Fish" is "entity", "enemy"
{
    public anim = 0;
    public speed = 60; // pixels per second

    actor = Actor("Fish");
    transform = Transform();
    enemy = Enemy();
    movement = DirectionalMovement();
    leftSensor = Sensor(-24, -16, 1, 32);
    rightSensor = Sensor(24, -16, 1, 32);

    state "main"
    {
        // setup
        movement.direction = randomDirection();
        movement.speed = Math.abs(speed);
        actor.anim = anim;
        actor.hflip = (movement.direction.x < 0);
        //leftSensor.visible = rightSensor.visible = true; // debug
        state = "hidden";
    }

    state "hidden"
    {
        // the fish will stay hidden until it is underwater
        if(transform.position.y < Level.waterlevel) {
            actor.visible = false;
            enemy.enabled = false;
            movement.enabled = false;
        }
        else {
            actor.visible = true;
            enemy.enabled = true;
            movement.enabled = true;
            state = "active";
        }
    }

    state "active"
    {
        // is the fish still underwater?
        if(transform.position.y >= Level.waterlevel) {
            // set swim direction
            if(leftSensor.status == "solid") {
                movement.direction = Vector2.right;
                actor.hflip = false;
            }
            else if(rightSensor.status == "solid") {
                movement.direction = Vector2.left;
                actor.hflip = true;
            }
        }
        else
            state = "hidden";
    }

    fun randomDirection()
    {
        x = Math.round(Math.random()) * 2 - 1;
        return Vector2(x, 0);
    }

    fun onReset()
    {
        state = "hidden";
    }
}