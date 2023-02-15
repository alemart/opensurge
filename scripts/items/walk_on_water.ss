// -----------------------------------------------------------------------------
// File: walk_on_water.ss
// Description: Walk on Water gimmick
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Brick;
using SurgeEngine.Level;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Collisions.CollisionBox;

//
// Walk on Water gimmick
//
// Walk on Water (or rather, Run on Water ;) is used to let the character run
// on the surface of the water if it has enough speed.
//
// Properties:
// - minSpeed: number. The minimum speed required to let the player walk on
//   water. This value is given in pixels per second and is always positive.
//   The default speed is generally good enough, and you may just keep it.
//

// this object should be placed on the left side of the surface of the water
object "Walk on Water Left" is "gimmick", "special", "entity"
{
    wow = spawn("Walk on Water").setEnteringDirection(1);

    fun get_minSpeed()
    {
        return wow.minSpeed;
    }

    fun set_minSpeed(minSpeed)
    {
        wow.minSpeed = minSpeed;
    }
}

// this object should be placed on the right side of the surface of the water
object "Walk on Water Right" is "gimmick", "special", "entity"
{
    wow = spawn("Walk on Water").setEnteringDirection(-1);

    fun get_minSpeed()
    {
        return wow.minSpeed;
    }

    fun set_minSpeed(minSpeed)
    {
        wow.minSpeed = minSpeed;
    }
}

object "Walk on Water" is "private", "gimmick", "special", "entity"
{
    public minSpeed = 600; // pixels per second
    collider = CollisionBox(28, 32);
    watchers = []; // keep the references to prevent garbage collection
    enteringDirection = 1; // +1 or -1

    fun onCollision(otherCollider)
    {
        if(otherCollider.entity.hasTag("player")) {
            player = otherCollider.entity;
            if(canWalkOnWater(player))
                walkOnWater(player);
        }
    }

    fun walkOnWater(player)
    {
        if(player.gsp * enteringDirection > 0) {
            // enter water
            if(null === player.child("Walk on Water Watcher")) {
                watcher = player.spawn("Walk on Water Watcher");
                watcher.minSpeed = minSpeed;
                watchers.push(watcher);
            }
        }
        else {
            // leave water
            watcher = player.child("Walk on Water Watcher");
            if(watcher !== null)
                watcher.destroy();
        }
    }

    fun canWalkOnWater(player)
    {
        return (
            !player.midair &&
            !player.underwater &&
            Math.abs(player.gsp) >= minSpeed
        );
    }

    fun setEnteringDirection(direction)
    {
        enteringDirection = direction;
        return this;
    }
}

object "Walk on Water Watcher" is "companion", "private", "entity"
{
    public minSpeed = 600; // pixels per second
    splashesPerSecond = 11;
    player = parent;
    brick = spawn("Walk on Water Brick");

    state "main"
    {
        // keep the minimum speed if rolling or running
        if(player.rolling || player.running) {
            if(player.slope == 0) {
                if(Math.abs(player.gsp) < minSpeed)
                    player.gsp = Math.sign(player.gsp) * minSpeed;
            }
        }

        // stop walking on water
        if(!canRemainWalkingOnWater(player)) {
            destroy();
            return;
        }

        // splash
        if(timeout(1.0 / splashesPerSecond))
            state = "splash";
    }

    state "splash"
    {
        position = Vector2(player.collider.center.x, Level.waterlevel);
        Level.spawnEntity("Walk on Water Splash", position);
        state = "main";
    }

    fun canRemainWalkingOnWater(player)
    {
        return (
            !player.midair &&
            !player.underwater &&
            Math.abs(player.gsp) >= minSpeed //&& player.slope == 0
        );
    }
}

object "Walk on Water Brick" is "private", "awake", "entity"
{
    //actor = Actor("Walk on Water Brick");
    brick = Brick("Walk on Water Brick");
    transform = Transform();

    state "main"
    {
        dy = Level.waterlevel - transform.position.y;
        transform.translateBy(0, dy);
        //transform.angle = 0;
    }

    fun constructor()
    {
        brick.type = "solid";
    }
}

object "Walk on Water Splash" is "private", "disposable", "entity"
{
    actor = Actor("Walk on Water Splash");
    sfx = Sound("samples/walk_on_water.wav");

    state "main"
    {
        if(actor.animation.finished)
            destroy();
    }

    fun constructor()
    {
        actor.zindex = 0.99;
        sfx.play();
    }
}