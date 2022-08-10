// -----------------------------------------------------------------------------
// File: walk_on_water.ss
// Description: Walk on Water gimmick
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Level;
using SurgeEngine.Vector2;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Collisions.CollisionBox;

//
// Walk on Water gimmick
//
// Walk on Water (or rather, Run on Water ;) is used to let the character run
// on the surface of the water if it has enough speed (see property minSpeed).
// Cloud bricks positioned at the level of the water and at a colored layer
// of your choice (see property layer) keep the character from falling,
// provided that a high speed is maintained.
//
// Properties:
// - minSpeed: number. The minimum speed required to let the player walk on
//   water. This value is given in pixels per second and is always positive.
//   The default speed is generally good enough, and you may just keep it.
// - layer: string. The layer of the bricks: either "yellow" or "green".
//   The default layer is "yellow". Keep the default layer unless you have
//   a specific reason for changing it.
//

// this object should be placed on the left side of the surface of the water
object "Walk on Water Left" is "special", "entity"
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

    fun get_layer()
    {
        return wow.layer;
    }

    fun set_layer(layer)
    {
        wow.layer = layer;
    }
}

// this object should be placed on the right side of the surface of the water
object "Walk on Water Right" is "special", "entity"
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

    fun get_layer()
    {
        return wow.layer;
    }

    fun set_layer(layer)
    {
        wow.layer = layer;
    }
}

object "Walk on Water" is "private", "special", "entity"
{
    public minSpeed = 600; // pixels per second
    public layer = "yellow";
    collider = CollisionBox(32, 32);
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
            if(null === player.child("Walk on Water - Player Watcher")) {
                watcher = player.spawn("Walk on Water - Player Watcher");
                watcher.minSpeed = minSpeed;
                watcher.layer = layer;
                watchers.push(watcher);
            }
        }
        else {
            // leave water
            watcher = player.child("Walk on Water - Player Watcher");
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

object "Walk on Water - Player Watcher" is "companion"
{
    public minSpeed = 600; // pixels per second
    public layer = "yellow";
    splashesPerSecond = 11;
    player = parent;

    state "main"
    {
        player.layer = layer;
        state = "watching";
    }

    state "watching"
    {
        if(!canRemainWalkingOnWater(player)) {
            destroy();
            return;
        }

        if(timeout(1.0 / splashesPerSecond))
            state = "splash";
    }

    state "splash"
    {
        position = Vector2(player.collider.center.x, Level.waterlevel);
        Level.spawnEntity("Walk on Water Splash", position);
        state = "watching";
    }

    fun destructor()
    {
        player.layer = "default";
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