// -----------------------------------------------------------------------------
// File: surge_the_rabbit.ss
// Description: a script specific to the Surge the Rabbit game
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine;
using SurgeEngine.Web;
using SurgeEngine.Game;
using SurgeEngine.Lang;
using SurgeEngine.Level;
using SurgeEngine.Platform;

@Package
object "SurgeTheRabbit"
{
    public readonly Settings = spawn("SurgeTheRabbitSettings");
    public readonly website = "https://opensurge2d.org";
    engineVersion = "";

    fun donate()
    {
        if(!canAcceptDonations())
            return;

        openWebsite("/contribute", {});
    }

    fun share()
    {
        if(Platform.isAndroid) {
            text = "Surge the Rabbit " + website;
            Platform.Android.shareText(text);
        }
        else {
            openWebsite("/share", {});
        }
    }

    fun download()
    {
        openWebsite("/download", {
            "type": Platform.isAndroid ? "desktop" : "mobile"
        });
    }

    fun submitFeedback()
    {
        if(!isBaseGame())
            return;

        if(Platform.isAndroid) {
            if(isBuild("googleplay")) {
                Web.launchURL("https://play.google.com/store/apps/details?id=org.opensurge2d.surgeengine");
                return;
            }
        }

        openWebsite("/feedback", {
            "platform": platformName()
        });
    }

    fun reportIssue()
    {
        if(!isBaseGame())
            return;

        openWebsite("/issues", {
            "platform": platformName()
        });
    }

    fun openWebsite(path, params)
    {
        url = website + path;
        url += "?v=" + Web.encodeURIComponent(SurgeEngine.version);
        url += "&lang=" + Web.encodeURIComponent(Lang["LANG_ID"]);
        url += "&from=" + Web.encodeURIComponent(Level.name);
        foreach(param in params)
            url += "&" + param.key + "=" + Web.encodeURIComponent(param.value);

        Web.launchURL(url);
    }

    fun isBaseGame()
    {
        return Game.title == "Surge the Rabbit" && Game.version == engineVersion;
    }

    fun platformName()
    {
        if(Platform.isWindows)
            return "windows";
        else if(Platform.isMacOS)
            return "macos";
        else if(Platform.isAndroid)
            return "android";
        else if(Platform.isUnix)
            return "unix";
        else
            return "unknown";
    }

    fun isDevelopmentBuild()
    {
        return SurgeEngine.version.indexOf("-dev") >= 0;
    }

    fun isBuild(buildName)
    {
        return SurgeEngine.version.indexOf(buildName) >= 0;
    }

    fun canAcceptDonations()
    {
        if(Platform.isAndroid) {
            /* no external donation page */
            if(isBuild("googleplay"))
                return false;
        }

        return isBaseGame();
    }

    fun constructor()
    {
        // find the engine version x.y.z[.w]
        engineVersion = SurgeEngine.version;
        if((j = engineVersion.indexOf("-")) >= 0)
            engineVersion = engineVersion.substr(0, j);

        // print a reminder
        if(Game.title == "Surge the Rabbit" && Game.version != engineVersion) {
            Console.print("SurgeEngine.version: " + SurgeEngine.version);
            Console.print("Game.version: " + Game.version);
        }
    }
}

object "SurgeTheRabbitSettings"
{
    public wantNeonAsPlayer2 = false;
}