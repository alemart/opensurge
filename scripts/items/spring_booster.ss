// -----------------------------------------------------------------------------
// File: spring_booster.ss
// Description: spring booster script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Brick;
using SurgeEngine.Player;
using SurgeEngine.Vector2;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Collisions.CollisionBox;

object "Spring Booster Right" is "entity", "basic"
{
    booster = spawn("Spring Booster").setDirection(1);

    state "main"
    {
    }

    fun onReset()
    {
        // "Spring Booster" is not spawned via the editor;
        // delegate responsibility
        booster.onReset();
    }
}

object "Spring Booster Left" is "entity", "basic"
{
    booster = spawn("Spring Booster").setDirection(-1);

    state "main"
    {
    }

    fun onReset()
    {
        booster.onReset();
    }
}

object "Spring Booster" is "private", "entity"
{
    boostSpeed = 960; // pixels per second
    actor = Actor("Spring Booster");
    brick = Brick("Spring Booster Block");
    sensorCollider = CollisionBox(96, 40);
    springCollider = CollisionBox(16, 32);
    boostSfx = Sound("samples/spring.wav");
    appearSfx = Sound("samples/trotada.wav");
    direction = 1; // 1: right, -1: left

    state "main"
    {
        springCollider.setAnchor(1 - (1 + direction) / 2, 1);
        brick.offset = Vector2(-16 * direction, 0);
        brick.enabled = false;
        changeAnimation(3);
        state = "invisible";
    }

    state "invisible"
    {
    }

    state "appearing"
    {
        if(actor.animation.finished) {
            changeAnimation(0);
            state = "visible";
        }
    }

    state "visible"
    {
    }

    state "boosting"
    {
        if(actor.animation.finished) {
            changeAnimation(0);
            state = "visible";
        }
    }

    fun setDirection(dir)
    {
        direction = Math.sign(dir);
        return this;
    }

    fun constructor()
    {
        brick.type = "cloud";
        sensorCollider.setAnchor(0.5, 1);
        springCollider.setAnchor(0, 1);
        //sensorCollider.visible = true;
        //springCollider.visible = true;
    }

    fun appear()
    {
        brick.enabled = true;
        appearSfx.play();
        changeAnimation(1);
        state = "appearing";
    }

    fun boost(player)
    {
        // change speed
        oldSpeed = Math.abs(player.speed);
        if(boostSpeed > oldSpeed)
            player.speed = direction * boostSpeed;

        // misc
        boostSfx.play();
        changeAnimation(2);
        state = "boosting";
    }

    fun changeAnimation(baseID)
    {
        actor.anim = baseID + ((1 - direction) / 2) * 5;
    }

    fun onCollision(otherCollider)
    {
        if(otherCollider.entity.hasTag("player")) {
            if(otherCollider.collidesWith(springCollider)) {
                if(state != "invisible")
                    boost(otherCollider.entity);
            }
            else if(state == "invisible")
                appear();
        }
    }

    fun onReset()
    {
        state = "main";
    }
}