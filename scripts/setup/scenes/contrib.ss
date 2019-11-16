// -----------------------------------------------------------------------------
// File: contrib.ss
// Description: sets up the launching of a contribution page on exit
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine;
using SurgeEngine.Level;
using SurgeEngine.Prefs;
using SurgeEngine.Lang;
using SurgeEngine.Web;

object "Contribute Page"
{
    state "main"
    {
        Application.onExit = Application.spawn("On Exit");
        Level.loadNext();
        state = "done";
    }

    state "done"
    {
    }
}

// this is a function object called on exit
// it's similar to atexit() from the C standard library
object "On Exit"
{
    url = "http://opensurge2d.org/contribute?v=" + SurgeEngine.version + "&lang=" + Lang["LANG_ID"];
    contrib = "contrib-" + SurgeEngine.version;
    contribInterval = 31536000; // 1 year

    fun call()
    {
        lastContrib = Number(Prefs[contrib] || 0);
        if(Date.unixtime >= lastContrib + contribInterval) {
            Prefs[contrib] = Date.unixtime;
            Web.launchURL(url);
        }
    }
}