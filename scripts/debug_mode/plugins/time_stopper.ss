// -----------------------------------------------------------------------------
// File: time_stopper.ss
// Description: time stopper plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*

This plugin stops time while the Debug Mode is activated.

*/

using SurgeEngine.Level;

object "Debug Mode - Time Stopper" is "debug-mode-plugin"
{
    frozenTime = 0;

    state "main"
    {
        Level.time = 0;
    }

    fun onLoad(debugMode)
    {
        frozenTime = Level.time;
    }

    fun onUnload(debugMode)
    {
        Level.time = frozenTime;
    }
}