// -----------------------------------------------------------------------------
// File: tubes.ss
// Description: tube system
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;
using SurgeEngine.Player;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Collisions.CollisionBox;

// Tube Out unlocks the player
object "Tube Out" is "entity", "special"
{
    tube = spawn("Tube");
    maxSpeed = 960;

    fun onTubeCollision(player)
    {
        // cap the speed when entering the tube
        if(player.input.enabled)
            player.speed = Math.clamp(player.speed, -maxSpeed, maxSpeed);

        // tube logic
        if(player.rolling) {
            // Tube In guarantees that
            // player.input.enabled == false
            // just before this occurs
            player.input.enabled = !player.input.enabled;
        }
        else {
            // walking?
            player.input.enabled = true;
        }
    }
}

// Tube In locks the player
object "Tube In" is "entity", "special"
{
    tube = spawn("Tube");

    fun onTubeCollision(player)
    {
        player.input.enabled = false;
        Level.spawn("Tube - Player Watcher").setPlayer(player);
    }
}

// Tube rolls the player
object "Tube" is "private", "special", "entity"
{
    public rollSpeed = 600;
    collider = CollisionBox(32, 32);
    sfx = Sound("samples/tube.wav");
    player = null;

    state "main"
    {
    }

    state "roll"
    {
        // roll
        if(!player.rolling)
            sfx.play();
        player.roll();

        // back to the main state
        state = "main";
    }

    fun onCollision(otherCollider)
    {
        if(otherCollider.entity.hasTag("player")) {
            player = otherCollider.entity;
            if(shouldBoost(player))
                boost(player);
            parent.onTubeCollision(player);
            state = "roll";
        }
    }

    fun boost(player)
    {
        if(player.speed >= 0)
            player.speed = Math.max(rollSpeed, player.speed);
        else
            player.speed = Math.min(-rollSpeed, player.speed);
    }

    fun shouldBoost(player)
    {
        return player.ysp >= 0;
    }
}

// An object that watches the player while inside the tube
object "Tube - Player Watcher" // it's like an awake entity
{
    player = null;

    state "main"
    {
        // prevent soft lock
        if(Math.abs(player.gsp) < 30 && !player.rolling)
            player.gsp = 300 * player.direction;

        // the character may get unrolled if it falls inside the tube
        if(!player.rolling)
            player.roll();

        // the player is out of the tube: we're done
        if(player.input.enabled)
            destroy();
    }

    fun setPlayer(p)
    {
        player = p;
        return this;
    }
}