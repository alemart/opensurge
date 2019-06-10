// -----------------------------------------------------------------------------
// File: springs.ss
// Description: spring scripting
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Player;
using SurgeEngine.Vector2;
using SurgeEngine.Collisions.CollisionBox;



//
// A spring is an entity composed of two objects:
//
// Spring Graphic - controls the graphics
// Spring Behavior - controls the logic
//
// Configure the objects differently, and you'll
// have a ton of different springs.
//



//
// Standard Springs
//

object "Spring Standard" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Standard")
        .setIdleAnimation(0)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(600)
        .setDirection(0, -1)
        .setSize(32, 16)
        .setAnchor(0.5, 1)
        .setSensitivity(false);

    state "main"
    {
    }

    fun onSpringActivate(player)
    {
    }
}

object "Spring Standard Top Right" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Standard Top Right")
        .setIdleAnimation(0)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(600)
        .setDirection(1, -1)
        .setSize(22, 22)
        .setAnchor(0, 1)
        .setSensitivity(true);

    state "main"
    {
    }
}

object "Spring Standard Right" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Standard Right")
        .setIdleAnimation(0)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(600)
        .setDirection(1, 0)
        .setSize(16, 32)
        .setAnchor(0, 0.5)
        .setSensitivity(true);

    state "main"
    {
    }
}

object "Spring Standard Bottom Right" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Standard Bottom Right")
        .setIdleAnimation(0)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(600)
        .setDirection(1, 1)
        .setSize(22, 22)
        .setAnchor(0, 0)
        .setSensitivity(true);

    state "main"
    {
    }
}

object "Spring Standard Bottom" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Standard Bottom")
        .setIdleAnimation(0)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(600)
        .setDirection(0, 1)
        .setSize(32, 16)
        .setAnchor(0.5, 0)
        .setSensitivity(true);

    state "main"
    {
    }
}

object "Spring Standard Bottom Left" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Standard Bottom Left")
        .setIdleAnimation(0)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(600)
        .setDirection(-1, 1)
        .setSize(22, 22)
        .setAnchor(1, 0)
        .setSensitivity(true);

    state "main"
    {
    }
}

object "Spring Standard Left" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Standard Left")
        .setIdleAnimation(0)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(600)
        .setDirection(-1, 0)
        .setSize(16, 32)
        .setAnchor(1, 0.5)
        .setSensitivity(true);

    state "main"
    {
    }
}

object "Spring Standard Top Left" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Standard Top Left")
        .setIdleAnimation(0)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(600)
        .setDirection(-1, -1)
        .setSize(22, 22)
        .setAnchor(1, 1)
        .setSensitivity(true);

    state "main"
    {
    }
}

object "Spring Standard Hidden" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Standard Hidden")
        .setIdleAnimation(2)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(600)
        .setDirection(0, -1)
        .setSize(32, 16)
        .setAnchor(0.5, 1)
        .setSensitivity(true);

    state "main"
    {
    }
}



//
// Stronger Springs
//

object "Spring Stronger" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Stronger")
        .setIdleAnimation(0)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(960)
        .setDirection(0, -1)
        .setSize(32, 16)
        .setAnchor(0.5, 1)
        .setSensitivity(false);

    state "main"
    {
    }

    fun onSpringActivate(player)
    {
    }
}

object "Spring Stronger Top Right" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Stronger Top Right")
        .setIdleAnimation(0)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(960)
        .setDirection(1, -1)
        .setSize(22, 22)
        .setAnchor(0, 1)
        .setSensitivity(true);

    state "main"
    {
    }
}

object "Spring Stronger Right" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Stronger Right")
        .setIdleAnimation(0)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(960)
        .setDirection(1, 0)
        .setSize(16, 32)
        .setAnchor(0, 0.5)
        .setSensitivity(true);

    state "main"
    {
    }
}

object "Spring Stronger Bottom Right" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Stronger Bottom Right")
        .setIdleAnimation(0)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(960)
        .setDirection(1, 1)
        .setSize(22, 22)
        .setAnchor(0, 0)
        .setSensitivity(true);

    state "main"
    {
    }
}

object "Spring Stronger Bottom" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Stronger Bottom")
        .setIdleAnimation(0)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(960)
        .setDirection(0, 1)
        .setSize(32, 16)
        .setAnchor(0.5, 0)
        .setSensitivity(true);

    state "main"
    {
    }
}

object "Spring Stronger Bottom Left" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Stronger Bottom Left")
        .setIdleAnimation(0)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(960)
        .setDirection(-1, 1)
        .setSize(22, 22)
        .setAnchor(1, 0)
        .setSensitivity(true);

    state "main"
    {
    }
}

object "Spring Stronger Left" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Stronger Left")
        .setIdleAnimation(0)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(960)
        .setDirection(-1, 0)
        .setSize(16, 32)
        .setAnchor(1, 0.5)
        .setSensitivity(true);

    state "main"
    {
    }
}

object "Spring Stronger Top Left" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Stronger Top Left")
        .setIdleAnimation(0)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(960)
        .setDirection(-1, -1)
        .setSize(22, 22)
        .setAnchor(1, 1)
        .setSensitivity(true);

    state "main"
    {
    }
}

object "Spring Stronger Hidden" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Stronger Hidden")
        .setIdleAnimation(2)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(960)
        .setDirection(0, -1)
        .setSize(32, 16)
        .setAnchor(0.5, 1)
        .setSensitivity(true);

    state "main"
    {
    }
}



//
// Strongest Springs
//

object "Spring Strongest" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Strongest")
        .setIdleAnimation(0)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(1320)
        .setDirection(0, -1)
        .setSize(32, 16)
        .setAnchor(0.5, 1)
        .setSensitivity(false);

    state "main"
    {
    }

    fun onSpringActivate(player)
    {
    }
}

object "Spring Strongest Top Right" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Strongest Top Right")
        .setIdleAnimation(0)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(1320)
        .setDirection(1, -1)
        .setSize(22, 22)
        .setAnchor(0, 1)
        .setSensitivity(true);

    state "main"
    {
    }
}

object "Spring Strongest Right" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Strongest Right")
        .setIdleAnimation(0)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(1320)
        .setDirection(1, 0)
        .setSize(16, 32)
        .setAnchor(0, 0.5)
        .setSensitivity(true);

    state "main"
    {
    }
}

object "Spring Strongest Bottom Right" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Strongest Bottom Right")
        .setIdleAnimation(0)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(1320)
        .setDirection(1, 1)
        .setSize(22, 22)
        .setAnchor(0, 0)
        .setSensitivity(true);

    state "main"
    {
    }
}

object "Spring Strongest Bottom" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Strongest Bottom")
        .setIdleAnimation(0)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(1320)
        .setDirection(0, 1)
        .setSize(32, 16)
        .setAnchor(0.5, 0)
        .setSensitivity(true);

    state "main"
    {
    }
}

object "Spring Strongest Bottom Left" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Strongest Bottom Left")
        .setIdleAnimation(0)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(1320)
        .setDirection(-1, 1)
        .setSize(22, 22)
        .setAnchor(1, 0)
        .setSensitivity(true);

    state "main"
    {
    }
}

object "Spring Strongest Left" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Strongest Left")
        .setIdleAnimation(0)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(1320)
        .setDirection(-1, 0)
        .setSize(16, 32)
        .setAnchor(1, 0.5)
        .setSensitivity(true);

    state "main"
    {
    }
}

object "Spring Strongest Top Left" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Strongest Top Left")
        .setIdleAnimation(0)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(1320)
        .setDirection(-1, -1)
        .setSize(22, 22)
        .setAnchor(1, 1)
        .setSensitivity(true);

    state "main"
    {
    }
}

object "Spring Strongest Hidden" is "entity", "gimmick"
{
    gfx = spawn("Spring Graphic")
        .setSprite("Spring Strongest Hidden")
        .setIdleAnimation(2)
        .setActiveAnimation(1);

    spring = spawn("Spring Behavior")
        .setSpeed(1320)
        .setDirection(0, -1)
        .setSize(32, 16)
        .setAnchor(0.5, 1)
        .setSensitivity(true);

    state "main"
    {
    }
}



//
// Spring Components
// (where magic happens ;)
//

object "Spring Graphic" is "private", "entity"
{
    actor = null;
    idleAnim = 0;
    activeAnim = 1;

    state "main"
    {
        if(actor != null)
            state = "idle";
    }

    state "idle"
    {
        actor.anim = idleAnim;
    }

    state "active"
    {
        actor.anim = activeAnim;
        if(actor.animation.finished)
            state = "idle";
    }

    fun onSpringActivate(player)
    {
        if(actor != null)
            state = "active";
    }



    // --- MODIFIERS ---

    fun setSprite(spriteName)
    {
        actor = Actor(spriteName);
        return this;
    }

    fun setIdleAnimation(animationNumber)
    {
        idleAnim = animationNumber;
        return this;
    }

    fun setActiveAnimation(animationNumber)
    {
        activeAnim = animationNumber;
        return this;
    }
}

object "Spring Behavior" is "private", "entity"
{
    sfx = Sound("samples/spring.wav");
    collider = CollisionBox(32, 16);
    direction = Vector2.up;
    speed = 600; // 960;
    sensitive = false;

    state "main"
    {
    }

    fun constructor()
    {
        collider.setAnchor(0.5, 1);
        //collider.visible = true;
    }

    fun onCollision(otherCollider)
    {
        if(otherCollider.entity.hasTag("player"))
            activateSpring(otherCollider.entity);
    }

    fun activateSpring(player)
    {
        if(player.ysp > 0 || sensitive) {
            // compute the velocity
            v = direction.scaledBy(speed);

            // boost player
            if(direction.x != 0) {
                if(v.x > 0 && v.x > player.speed)
                    player.speed = v.x;
                else if(v.x < 0 && v.x < player.speed)
                    player.speed = v.x;
            }
            if(direction.y != 0) {
                if(v.y > 0 && v.y > player.speed)
                    player.ysp = v.y;
                else if(v.y < 0 && v.y < player.speed)
                    player.ysp = v.y;
            }

            // prevent braking
            if(direction.x != 0)
                player.hlock(0.27);

            // change mode
            if(direction.y != 0)
                player.springify();

            // play sound
            if(sfx != null)
                sfx.play();

            // notify parent & graphic sibling
            if((gfx = sibling("Spring Graphic")) != null)
                gfx.onSpringActivate(player);
            if(parent.hasFunction("onSpringActivate"))
                parent.onSpringActivate(player);
        }
    }



    // --- MODIFIERS ---

    // set the speed, in px/s
    fun setSpeed(spd)
    {
        speed = Math.abs(spd);
        return this;
    }

    // set the direction of the spring
    fun setDirection(x, y)
    {
        direction = Vector2(x, y).normalized();
        return this;
    }

    // set the size of the collider
    // width, height given in pixels
    fun setSize(width, height)
    {
        collider.width = width;
        collider.height = height;
        return this;
    }

    // set the anchor of the collider
    // normally, 0 <= x, y <= 1
    fun setAnchor(x, y)
    {
        collider.setAnchor(x, y);
        return this;
    }

    // make the spring "sensitive" to touch
    // (no need to jump on it)
    fun setSensitivity(isSensitive)
    {
        sensitive = isSensitive;
        return this;
    }

    // set the sound of the spring
    // path must be the path of a sound (or null)
    // e.g., "samples/spring.wav"
    fun setSound(path)
    {
        sfx = (path !== null) ? Sound(path) : null;
        return this;
    }
}