// -----------------------------------------------------------------------------
// File: exit.ss
// Description: exit plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Input;

object "Debug Mode - Exit" is "debug-mode-plugin"
{
    debugMode = parent;
    input = Input("default");
    backButton = "fire4";

    state "main"
    {
        // return to the game if the back button is pressed
        if(input.buttonPressed(backButton))
            debugMode.exit();
    }
}