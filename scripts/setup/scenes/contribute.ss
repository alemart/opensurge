// -----------------------------------------------------------------------------
// File: contribute.ss
// Description: sets up the launching of a donation page when exiting the game
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine;
using SurgeEngine.Level;
using SurgeEngine.Prefs;
using SurgeEngine.Platform;
using SurgeTheRabbit;

// Setup object
object "Setup contribute web page"
{
    state "main"
    {
        Application.onExit = Application.spawn("Open Contribute web page");
        Level.loadNext();
        state = "done";
    }

    state "done"
    {
    }
}

// this is a function object called when exiting the game
// it's a bit like using atexit() from the C standard library
object "Open Contribute web page"
{
    key = "contrib-" + SurgeEngine.version;

    fun call()
    {
        if(!SurgeTheRabbit.canAcceptDonations())
            return;

        asked = Prefs[key] || false;
        if(!asked) {
            Prefs[key] = true;
            SurgeTheRabbit.donate();
        }
    }
}