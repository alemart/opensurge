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
using SurgeEngine.Player;
using SurgeEngine.Platform;

@Package
object "SurgeTheRabbit"
{
    public readonly GameState = spawn("SurgeTheRabbit - Game State");
    public readonly Settings = spawn("SurgeTheRabbit - Settings");
    public readonly website = "https://opensurge2d.org";
    engineVersion = "";

    fun visitHomepage()
    {
        openWebsite("/", {});
    }

    fun donate()
    {
        if(!canAcceptDonations())
            return;

        openWebsite("/contribute", {});
    }

    fun share()
    {
        if(Platform.isAndroid)
            Platform.Android.shareText(website);
        else
            openWebsite("/share", {});
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
        return isBuild("-dev");
    }

    fun isBuild(buildName)
    {
        return SurgeEngine.version.indexOf(buildName) >= 0;
    }

    fun canAcceptDonations()
    {
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

object "SurgeTheRabbit - Settings"
{
    public wantNeonAsPlayer2 = false;
}

object "SurgeTheRabbit - Game State"
{
    consumableEntityTracker = spawn("SurgeTheRabbit - Consumable Entity Tracker");

    state "main"
    {
        reset();
        state = "done";
    }

    state "done"
    {
    }

    fun reset()
    {
        // reset components
        consumableEntityTracker.reset();

        // reset player data
        player = Player.active;
        player.lives = Player.initialLives;
        player.score = 0;
    }

    fun consumeEntity(entity)
    {
        consumableEntityTracker.consume(entity);
    }

    fun isEntityConsumed(entity)
    {
        return consumableEntityTracker.isConsumed(entity);
    }
}

object "SurgeTheRabbit - Consumable Entity Tracker"
{
    /*

    All level entities are flagged consumed or not consumed. Initially, none
    are consumed. Any entity may be flagged consumed. A consumable entity is an
    entity that will become consumed under some circumstances. Consumed entities
    retain the flag until the player moves to a different level. Level restarts
    do not affect the flag. Only by changing the level a consumed entity will
    become not consumed.

    Life Powerups are examples of consumable entities.

    */

    watchedLevelFile = "";
    consumedEntities = [];

    state "main"
    {
        // reset the state on level change
        if(watchedLevelFile != Level.file) {
            watchedLevelFile = Level.file;
            reset();
        }
    }

    // consume an entity
    fun consume(entity)
    {
        entityId = Level.entityId(entity);

        if(String.isNullOrEmpty(entityId)) { // sanity check
            Console.print("Can't consume entity: invalid ID");
            return;
        }

        if(consumedEntities.indexOf(entityId) < 0)
            consumedEntities.push(entityId);
    }

    // check if an entity is consumed
    fun isConsumed(entity)
    {
        entityId = Level.entityId(entity);
        return consumedEntities.indexOf(entityId) >= 0;
    }

    // forcibly reset the state
    fun reset()
    {
        consumedEntities.clear();
    }
}