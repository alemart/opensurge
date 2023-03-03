// -----------------------------------------------------------------------------
// File: contribute.ss
// Description: sets up the launching of a donation page when exiting the game
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine;
using SurgeEngine.Level;
using SurgeEngine.Prefs;
using SurgeEngine.Lang;
using SurgeEngine.Web;

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
        if(isGooglePlayBuild())
            return;

        asked = Prefs[key] || false;
        if(!asked) {
            Prefs[key] = true;
            Web.launchURL(donateURL());
        }
    }

    fun donateURL()
    {
        return "http://opensurge2d.org/contribute?v=" + SurgeEngine.version + "&lang=" + Lang["LANG_ID"];
    }

    fun isGooglePlayBuild()
    {
        /* adding an external donation page to an Android app published
           in the Google Play Store is a violation of their policy */
        return SurgeEngine.version.indexOf("googleplay") >= 0;
    }
}