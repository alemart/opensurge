// -----------------------------------------------------------------------------
// File: player_locker.ss
// Description: player locker plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*

This plugin disables the physics and the interactions of the Player objects
while the Debug Mode is activated.

*/

using SurgeEngine.Player;

object "Debug Mode - Player Locker" is "debug-mode-plugin"
{
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

    fun onLoad(debugMode)
    {
        for(i = Player.count - 1; i >= 0; i--) {
            player = Player[i];
            frozenPhysics[player.id] = player.frozen;
            enabledInput[player.id] = player.input.enabled;
            enabledCollider[player.id] = player.collider.enabled;
        }
    }

    fun onUnload(debugMode)
    {
        for(i = Player.count - 1; i >= 0; i--) {
            player = Player[i];
            player.frozen = frozenPhysics[player.id];
            player.input.enabled = enabledInput[player.id];
            player.collider.enabled = enabledCollider[player.id];
        }
    }
}