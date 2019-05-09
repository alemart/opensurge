// -----------------------------------------------------------------------------
// File: bg_xchg.ss
// Description: background exchanger sprite script
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
    collider = CollisionBox(2, 32);
    player = null;
    manager = Level.child("Background Exchange Manager") || Level.spawn("Background Exchange Manager");

    state "main"
    {
    }

    state "watch collision"
    {
        if(!collider.collidesWith(player.collider)) {
            // change the background state
            if(player.speed > 0 || (player.speed == 0 && player.ysp > 0))
                manager.changeBackgroundOfPlayer(player, background);
            else
                manager.restoreBackgroundOfPlayer(player);

            // done
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
            playerName = Player(i).name;
            originalBackgroundOf[playerName] = Level.background;
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