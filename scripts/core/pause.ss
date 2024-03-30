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
    start = "fire3";
    back = "fire4";

    state "main"
    {
        if(!Level.cleared) {
            // handle the start and the back buttons
            if(input.buttonPressed(start) || input.buttonPressed(back))
                Level.pause();

            // handling the back button is required on Android; we enforce it
            // in all systems in order to maintain consistency
        }
    }
}

// the following object is kept for backwards compatibility with Open Surge 0.5.x
object "Pause and Quit"
{
    obj = spawn("Default Pause and Quit");
}
