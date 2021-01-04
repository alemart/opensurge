// -----------------------------------------------------------------------------
// File: setup.ss
// Description: the Default Setup object spawns: Default HUD, Default Camera,
//              Switch Controller, and so on. This is spawned by default on
//              every level, but that can be changed manually (see below)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Level;

//
// Need to hack this?
// It's not recommended to change this object. Instead, copy this object, give
// it a different name and change the setup field on the .lev file accordingly.
//
object "Default Setup"
{
    cam = spawn("Default Camera");
    switchController = spawn("Switch Controller");
    pauseController = spawn("Pause and Quit");
    waterController = spawn("Water Controller");
    clearedAnim = spawn("Default Cleared Animation");
    openingAnim = spawn("Default Opening Animation");
    animalManager = Level.spawn("Animals");
    collectiblesListener = spawn("Collectibles Listener").triggers("Give Extra Life").every(100);
    hud = null;
    player = null;

    state "main"
    {
        player = Player.active;
        player.input.enabled = false;
        state = "wait";
    }

    // wait for the completion of the opening animation
    state "wait"
    {
        if(timeout(3.0)) {
            hud = spawn("Default HUD");
            player.input.enabled = true;
            state = "done";
        }
    }

    state "done"
    {
    }
}

//
// A debug setup object without the opening animation.
// Useful for development & debugging.
//
object "Debug Setup"
{
    cam = spawn("Default Camera");
    switchController = spawn("Switch Controller");
    pauseController = spawn("Pause and Quit");
    waterController = spawn("Water Controller");
    clearedAnim = spawn("Default Cleared Animation");
    hud = spawn("Default HUD");
    animalManager = Level.spawn("Animals");
    collectiblesListener = spawn("Collectibles Listener").triggers("Give Extra Life").every(100);
}

// this is for retro-compatibility
object ".default_startup" { setup = spawn("Default Setup"); }
object "DefaultStartup" { setup = spawn("Default Setup"); }
