// -----------------------------------------------------------------------------
// File: startup.ss
// Description: the DefaultStartup object spawns: Default HUD, Default Camera,
//              Switch Controller, and so on. This is spawned by default on
//              every level, but that can be changed by hacking (see below)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

// Need to hack this?
// It's not recommended to change this object. You may want to copy this object,
// give it a different name and change the 'startup' parameter on the .lev file.
object "DefaultStartup"
{
    hud = spawn("DefaultHUD");
    cam = spawn("DefaultCamera");
    switchController = spawn("DefaultSwitchController");
    waterController = spawn("DefaultWaterController");
    clearedAnim = spawn("DefaultClearedAnimation");
    //openingAnim = spawn("DefaultOpeningAnimation");

    state "main"
    {
    }
}

// this is for retro-compatibility
object ".default_startup"
{
    defaultStartup = spawn("DefaultStartup");

    state "main"
    {
    }
}