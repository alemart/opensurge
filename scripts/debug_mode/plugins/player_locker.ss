// -----------------------------------------------------------------------------
// File: player_locker.ss
// Description: player locker plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;

object "Debug Mode - Player Locker" is "debug-mode-plugin"
{
    debugMode = parent;
    frozenPhysics = {};
    enabledInput = {};
    enabledCollider = {};

    state "main"
    {
        for(i = Player.count - 1; i >= 0; i--) {
            player = Player[i];
            player.frozen = true;
            player.input.enabled = false;
            player.collider.enabled = false;
        }
    }

    fun constructor()
    {
        for(i = Player.count - 1; i >= 0; i--) {
            player = Player[i];
            frozenPhysics[player.name] = player.frozen;
            enabledInput[player.name] = player.input.enabled;
            enabledCollider[player.name] = player.collider.enabled;
        }
    }

    fun destructor()
    {
        for(i = Player.count - 1; i >= 0; i--) {
            player = Player[i];
            player.frozen = frozenPhysics[player.name];
            player.input.enabled = enabledInput[player.name];
            player.collider.enabled = enabledCollider[player.name];
        }
    }
}