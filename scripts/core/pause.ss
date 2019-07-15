// -----------------------------------------------------------------------------
// File: pause.ss
// Description: handles pause & quit
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;
using SurgeEngine.Player;

object "Pause and Quit"
{
    state "main"
    {
        if(!Level.cleared) {
            input = Player.active.input;
            if(input.buttonPressed("fire3"))
                Level.pause();
            else if(input.buttonPressed("fire4"))
                Level.quit();
        }
    }
}