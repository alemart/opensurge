// -----------------------------------------------------------------------------
// File: enable_character_switching.ss
// Description: a function object that enables character switching
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;

//
// Enable Character Switching is a function object that
// enables character switching
//
object "Enable Character Switching"
{
    fun call()
    {
        switchController = Level.findObject("Switch Controller");
        if(switchController != null)
            switchController.enabled = true;
        else
            Console.print("Can't enable character switching: missing Switch Controller");
    }
}