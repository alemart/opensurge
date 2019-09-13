// -----------------------------------------------------------------------------
// File: bg_xchg.ss
// Description: script of the background exchanger
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Level;
using SurgeEngine.Collisions.CollisionBox;

//
// Background Exchanger API
//
// Properties:
// - background: string. Path to a .bg file in the themes/ folder.
//
object "Background Exchanger" is "entity", "special"
{
    public background = "themes/template.bg";
    collider = CollisionBox(32, 32);
    player = null;
    manager = Level.child("Background Exchange Manager") || Level.spawn("Background Exchange Manager");

    state "main"
    {
    }

    state "watch collision"
    {
        if(!collider.collidesWith(player.collider)) {
            manager.changeBackgroundOfPlayer(player, background);
            state = "main";
        }
    }

    fun onCollision(otherCollider)
    {
        if(otherCollider.entity.hasTag("player")) {
            player = otherCollider.entity;
            state = "watch collision";
        }
    }
}

object "Background Restorer" is "entity", "special"
{
    collider = CollisionBox(32, 32);
    player = null;
    manager = Level.child("Background Exchange Manager") || Level.spawn("Background Exchange Manager");

    state "main"
    {
    }

    state "watch collision"
    {
        if(!collider.collidesWith(player.collider)) {
            manager.restoreBackgroundOfPlayer(player);
            state = "main";
        }
    }

    fun onCollision(otherCollider)
    {
        if(otherCollider.entity.hasTag("player")) {
            player = otherCollider.entity;
            state = "watch collision";
        }
    }
}

object "Background Exchange Manager" is "private", "awake", "entity"
{
    // this object simply keeps the state
    originalBackgroundOf = { };
    currentBackgroundOf = { };

    state "main"
    {
        // initialize
        for(i = 0; i < Player.count; i++) {
            playerName = Player[i].name;
            originalBackgroundOf[playerName] = Level.bgtheme;
            currentBackgroundOf[playerName] = Level.background;
        }

        // done
        state = "watch";
    }

    state "watch"
    {
        // update background on player change
        currentBackground = currentBackgroundOf[Player.active.name];
        if(Level.background != currentBackground && currentBackground)
            Level.background = currentBackground;
    }

    fun changeBackgroundOfPlayer(player, background)
    {
        currentBackgroundOf[player.name] = background;
    }

    fun restoreBackgroundOfPlayer(player)
    {
        currentBackgroundOf[player.name] = originalBackgroundOf[player.name];
    }
}