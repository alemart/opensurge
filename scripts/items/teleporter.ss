// -----------------------------------------------------------------------------
// File: teleporter.ss
// Description: teleporter script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Brick;
using SurgeEngine.Player;
using SurgeEngine.Transform;
using SurgeEngine.Audio.Sound;

object "Teleporter" is "entity", "gimmick"
{
    actor = Actor("Teleporter");
    brick = Brick("Teleporter Mask");
    sfx = Sound("samples/teleporter.wav");
    transform = Transform();
    selectedPlayers = [];

    state "main"
    {
    }

    state "teleporting"
    {
        player = Player.active;
        actor.anim = 1;
        player.input.enabled = false;
        if(timeout(3.0)) {
            actor.anim = 0;
            player.input.enabled = true;
            bringSelectedPlayers();
            state = "main";
        }
    }

    // teleport all players, except the active one
    fun teleport()
    {
        // select all players except the active one
        names = [];
        for(i = 0; i < Player.count; i++) {
            if(Player[i] != Player.active)
                names.push(Player[i].name);
        }

        // teleport the players
        return teleportPlayers(names);
    }

    // teleport a specific player
    fun teleportPlayer(playerName)
    {
        return teleportPlayers([playerName]);
    }

    // teleport specific players, given an array of player names
    fun teleportPlayers(playerNames)
    {
        // select players
        selectedPlayers.clear();
        foreach(name in playerNames) {
            player = Player(name);
            if(player != null)
                selectedPlayers.push(player);
        }

        // activate the teleporter
        if(selectedPlayers.length > 0) {
            sfx.play();
            state = "teleporting";
        }
    }

    // internal function
    fun bringSelectedPlayers()
    {
        n = selectedPlayers.length;
        position = transform.position;
        for(i = 0; i < n; i++) {
            player = selectedPlayers[i];
            player.transform.position = position.translatedBy(
                -16 * (n - 1) + 32 * i, -player.collider.height
            );
        }
    }
}