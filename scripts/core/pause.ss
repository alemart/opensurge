// -----------------------------------------------------------------------------
// File: pause.ss
// Description: handles pause & quit
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;
using SurgeEngine.Input;

object "Pause and Quit"
{
    input = Input("default");

    state "main"
    {
        if(!Level.cleared) {
            if(input.buttonPressed("fire3"))
                Level.pause();
            else if(input.buttonPressed("fire4"))
                Level.quit();
        }
    }
}