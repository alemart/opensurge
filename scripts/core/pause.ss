// -----------------------------------------------------------------------------
// File: pause.ss
// Description: handles pause & quit
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;
using SurgeEngine.Input;

object "Default Pause and Quit"
{
    input = Input("default");

    state "main"
    {
        if(!Level.cleared) {
            if(input.buttonPressed("fire3"))
                Level.pause();
            /*
            // the following is obsolete since Open Surge 0.6.1
            // (not helpful in terms of user experience)
            else if(input.buttonPressed("fire4"))
                Level.quit();
            */
        }
    }
}

// an alias kept for retro-compatiblity with Open Surge 0.5.x
object "Pause and Quit"
{
    obj = spawn("Default Pause and Quit");
}