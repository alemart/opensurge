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
    sfx = Sound("samples/tube.wav");
    maxSpeed = 960;
    boostSpeed = 600;

    fun onTubeCollision(player)
    {
        // cap the speed when entering the tube
        if(player.input.enabled)
            player.gsp = Math.clamp(player.gsp, -maxSpeed, maxSpeed);

        // tube logic
        if(player.rolling) {
            // Tube In guarantees that
            // player.input.enabled == false
            // just before this occurs
            player.input.enabled = !player.input.enabled;
        }
        else {
            // walking at the entrance of the tube?
            player.input.enabled = true;
        }

        // roll
        if(!player.rolling) {
            sfx.play();
            player.roll();
        }
    }

    fun onTubeOverlap(player)
    {
        // do nothing if the player is midair
        if(player.midair)
            return;

        // boost the player
        if(isOutsideTheTube(player))
            boost(player);
    }

    fun isOutsideTheTube(player)
    {
        return (player.child("Tube - Player Watcher") == null);
    }

    fun boost(player)
    {
        if(player.gsp >= 0)
            player.gsp = Math.max(boostSpeed, player.gsp);
        else
            player.gsp = Math.min(-boostSpeed, player.gsp);
    }
}

// Tube In locks the player
object "Tube In" is "entity", "special"
{
    tube = spawn("Tube");
    refs = [];

    fun onTubeCollision(player)
    {
        // disable input
        player.input.enabled = false;

        // spawn the watcher
        if(player.child("Tube - Player Watcher") == null) {
            watcher = player.spawn("Tube - Player Watcher");
            refs.push(watcher);
        }
    }

    fun onTubeOverlap(player)
    {
    }
}

// Base class
object "Tube" is "private", "special", "entity"
{
    collider = CollisionBox(32, 32);

    fun onCollision(otherCollider)
    {
        if(otherCollider.entity.hasTag("player")) {
            player = otherCollider.entity;
            parent.onTubeCollision(player);
        }
    }

    fun onOverlap(otherCollider)
    {
        if(otherCollider.entity.hasTag("player")) {
            player = otherCollider.entity;
            parent.onTubeOverlap(player);
        }
    }
}

// An object that watches the player while inside the tube
object "Tube - Player Watcher" is "special", "companion"
{
    player = parent;
    pushSpeed = 300;
    pushThreshold = 60;

    state "main"
    {
        // prevent soft lock by giving the player a little push if needed
        if(Math.abs(player.gsp) < pushThreshold)
            player.gsp = pushSpeed * Math.sign(player.gsp);

        // the player may get unrolled if he or she falls inside the tube
        if(!player.rolling)
            player.roll();

        // the player is out of the tube: we're done
        if(player.input.enabled)
            destroy();
    }
}