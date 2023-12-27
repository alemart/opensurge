// -----------------------------------------------------------------------------
// File: surge_the_rabbit.ss
// Description: a script specific to the Surge the Rabbit game
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine;
using SurgeEngine.Web;
using SurgeEngine.Game;
using SurgeEngine.Platform;

@Package
object "SurgeTheRabbit"
{
    public readonly website = "https://opensurge2d.org";

    fun donate()
    {
        if(!canAcceptDonations())
            return;

        url = website + "/contribute?v=" + SurgeEngine.version;
        Web.launchURL(url);
    }

    fun share()
    {
        if(Platform.isAndroid) {
            text = "Surge the Rabbit " + website;
            Platform.Android.shareText(text);
        }
        else {
            url = website + "/share?v=" + SurgeEngine.version;
            Web.launchURL(url);
        }
    }

    fun rate()
    {
        url = website + "/rating?v=" + SurgeEngine.version;
        Web.launchURL(url);
    }

    fun isBaseGame()
    {
        return (Game.title == "Surge the Rabbit");
    }

    fun isDevelopmentBuild()
    {
        return SurgeEngine.version.indexOf("-dev") >= 0;
    }

    fun isGooglePlayBuild()
    {
        return SurgeEngine.version.indexOf("googleplay") >= 0;
    }

    fun canAcceptDonations()
    {
        /* adding an external donation page to an Android app published
           in the Google Play Store is a violation of their policy */
        if(isGooglePlayBuild())
            return false;

        return isBaseGame();
    }

    fun constructor()
    {
        // print a reminder
        if(isBaseGame()) {
            if(SurgeEngine.version !== Game.version)
                Console.print("Outdated Game.version " + Game.version);
        }
    }
}