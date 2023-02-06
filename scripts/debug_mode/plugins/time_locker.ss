// -----------------------------------------------------------------------------
// File: time_locker.ss
// Description: time locker plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;

object "Debug Mode - Time Locker" is "debug-mode-plugin"
{
    debugMode = parent;
    frozenTime = Level.time;

    state "main"
    {
        Level.time = 0;
    }

    fun destructor()
    {
        Level.time = frozenTime;
    }
}