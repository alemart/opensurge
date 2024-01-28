// -----------------------------------------------------------------------------
// File: change_background.ss
// Description: a function object that changes the background
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;

//
// Change Background is a function object that changes the background.
//
// Arguments:
// - path: string. The path of the .bg file to be displayed
//                 (e.g., "themes/waterworks-zone-indoors.bg")
//
object "Change Background"
{
    fun call(path)
    {
        Level.background = path;
    }
}