// -----------------------------------------------------------------------------
// File: motd.ss
// Description: SurgeScript MOTD (Message Of The Day)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

// Greets the user with the Message Of The Day ;)
object "SurgeScript MOTD"
{
    state "main"
    {
        Console.print("Welcome to SurgeScript " + SurgeScript.version + "!");
        destroy();
    }
}
