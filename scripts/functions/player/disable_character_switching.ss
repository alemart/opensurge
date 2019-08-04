// -----------------------------------------------------------------------------
// File: disable_character_switching.ss
// Description: a function object that disables character switching
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;

//
// Disable Character Switching is a function object that
// disables character switching
//
object "Disable Character Switching"
{
    fun call()
    {
        switchController = Level.findObject("Switch Controller");
        if(switchController != null)
            switchController.enabled = false;
        else
            Console.print("Can't disable character switching: missing Switch Controller");
    }
}