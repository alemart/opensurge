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
    base = spawn("Background Exchanger Base");

    fun onBackgroundExchangerCallback(player, manager)
    {
        manager.changeBackgroundOfPlayer(player, background);
    }
}

object "Background Restorer" is "entity", "special"
{
    base = spawn("Background Exchanger Base");

    fun onBackgroundExchangerCallback(player, manager)
    {
        manager.restoreBackgroundOfPlayer(player);
    }
}

object "Background Exchanger Base" is "private", "entity"
{
    manager = Level.child("Background Exchange Manager") || Level.spawn("Background Exchange Manager");
    collider = CollisionBox(32, 32);
    player = null;

    state "main"
    {
    }

    state "watch collision"
    {
        if(!collider.collidesWith(player.collider)) {
            parent.onBackgroundExchangerCallback(player, manager);
            player = null;
            state = "main";
        }
    }

    fun onCollision(otherCollider)
    {
        if(otherCollider.entity.hasTag("player")) {
            if(player !== null) // if already watching (i.e., multiplayer)
                parent.onBackgroundExchangerCallback(player, manager);

            player = otherCollider.entity;
            state = "watch collision";
        }
    }
}

object "Background Exchange Manager"
{
    // this object simply keeps the state
    originalBackgroundOf = { };
    currentBackgroundOf = { };

    state "main"
    {
        // initialize
        for(i = 0; i < Player.count; i++) {
            playerId = Player[i].id;
            originalBackgroundOf[playerId] = Level.bgtheme;
            currentBackgroundOf[playerId] = Level.background;
        }

        // done
        state = "watch";
    }

    state "watch"
    {
        // update background on player change
        currentBackground = currentBackgroundOf[Player.active.id];
        if(Level.background != currentBackground && currentBackground)
            Level.background = currentBackground;
    }

    fun changeBackgroundOfPlayer(player, background)
    {
        currentBackgroundOf[player.id] = background;
    }

    fun restoreBackgroundOfPlayer(player)
    {
        currentBackgroundOf[player.id] = originalBackgroundOf[player.id];
    }

    fun backgroundOfPlayer(player)
    {
        return currentBackgroundOf[player.id];
    }
}