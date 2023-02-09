// -----------------------------------------------------------------------------
// File: default_camera_disabler.ss
// Description: Default Camera disabler plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*

This plugin disables the Default Camera during the time the Debug Mode is activated.

*/

using SurgeEngine.Level;

object "Debug Mode - Default Camera Disabler" is "debug-mode-plugin"
{
    defaultCamera = null;
    wasEnabled = true;

    state "main"
    {
        if(defaultCamera !== null)
            defaultCamera.enabled = false;
    }

    fun onLoad(debugMode)
    {
        defaultCamera = Level.findObject("Default Camera");
        if(defaultCamera !== null)
            wasEnabled = defaultCamera.enabled;
    }

    fun onUnload(debugMode)
    {
        if(defaultCamera !== null)
            defaultCamera.enabled = wasEnabled;
    }
}