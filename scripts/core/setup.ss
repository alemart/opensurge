// -----------------------------------------------------------------------------
// File: setup.ss
// Description: the Default Setup object spawns: Default HUD, Default Camera,
//              Switch Controller, and so on. This is spawned by default on
//              every level, but that can be changed manually (see below)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

//
// Need to hack this?
// It's not recommended to change this object. You may want to copy this object,
// give it a different name and change the "startup" parameter on the .lev file.
//
object "Default Setup"
{
    hud = null;
    cam = spawn("Default Camera");
    switchController = spawn("Switch Controller");
    pauseController = spawn("Pause and Quit");
    waterController = spawn("Water Controller");
    clearedAnim = spawn("Default Cleared Animation");
    openingAnim = spawn("Default Opening Animation");

    state "main"
    {
        // wait for the completion of the opening animation
        if(timeout(3.0)) {
            hud = spawn("Default HUD");
            state = "done";
        }
    }

    state "done"
    {
    }
}

// this is for retro-compatibility
object ".default_startup" { setup = spawn("Default Setup"); }
object "DefaultStartup" { setup = spawn("Default Setup"); }