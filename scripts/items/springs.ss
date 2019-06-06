// -----------------------------------------------------------------------------
// File: springs.ss
// Description: spring scripting
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Player;
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Collisions.CollisionBox;

object "Spring Standard Hidden" is "entity", "gimmick"
{
    actor = Actor("Spring Standard Hidden");
    spring = spawn("Spring Behavior")
        .setDirection(0, -1)
        .setStrength(1)
        .setSensitive()
    ;

    state "main"
    {
        actor.anim = 1;
    }

    state "active"
    {
        if(actor.animation.finished)
            state = "main";
    }

    fun onSpringActivate(player)
    {
        actor.anim = 0;
        state = "active";
    }
}

object "Spring Behavior" is "private", "entity"
{
    sfx = Sound("samples/spring.wav");
    collider = CollisionBox(32, 16);
    direction = Vector2.up;
    strength = 600; // 960;
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
        if(otherCollider.entity.hasTag("player")) {
            if(!sfx.playing) sfx.play();
            springify(otherCollider.entity);
        }
    }

    fun springify(player)
    {
        if(player.ysp > 0 || sensitive) {
            // compute the strength vector
            v = direction.scaledBy(strength);

            // hack
            if(sensitive && v.y < 0)
                player.transform.move(0, -20);

            // change mode
            player.springify();

            // boost player
            if(v.x != 0)
                player.speed = v.x;
            if(v.y != 0)
                player.ysp = v.y;

            // call parent
            if(parent.hasFunction("onSpringActivate"))
                parent.onSpringActivate(player);
        }
    }



    // --- MODIFIERS ---

    // set the strength of the spring
    // x: 1, 2, 3...
    fun setStrength(x)
    {
        s = Math.clamp(x, 1, 3) - 1;
        strength = 600 + 360 * s;
        return this;
    }

    // set the direction of the spring
    fun setDirection(x, y)
    {
        direction = Vector2(x, y).normalized();
        return this;
    }

    // set the anchor of the collider
    // normally, 0 <= x, y <= 1
    fun setAnchor(x, y)
    {
        collider.setAnchor(x, y);
        return this;
    }

    // make the spring "sensitive"
    fun setSensitive()
    {
        sensitive = true;
        return this;
    }
}