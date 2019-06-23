// -----------------------------------------------------------------------------
// File: animal.ss
// Description: little animal script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Behavior.Platformer;

// a generic little animal (usually spawned when defeating baddies)
object "Animal" is "entity", "private", "disposable"
{
    public id = 0; // animal ID: 0, 1, 2, 3...
    public secondsBeforeMoving = 0.0;

    animalCount = 12; // max animals; see the sprite
    actor = Actor("Animal");
    platformer = Platformer();

    state "main"
    {
        // play "appearing" animation
        actor.anim = 2 * Math.clamp(id, 0, animalCount - 1);

        // wait a bit
        state = "wait";
    }

    state "wait"
    {
        platformer.enabled = false;
        if(timeout(secondsBeforeMoving)) {
            platformer.enabled = true;
            platformer.propelUpward(240);
            state = "initial jump";
        }
    }

    state "initial jump"
    {
        // when touching the ground, run!
        if(!platformer.midair) {
            if(Math.random() < 0.5)
                platformer.walkLeft();
            else
                platformer.walkRight();

            actor.anim++; // play "running" animation
            state = "running";
        }
    }

    state "running"
    {
        if(!platformer.midair)
            platformer.jump();
    }

    fun constructor()
    {
        // select a random animal => id: 0, 1, ..., animalCount - 1
        id = Math.floor(animalCount * Math.random());

        // setup the platformer
        platformer.speed = 64;
        platformer.jumpSpeed = 200;
    }
}