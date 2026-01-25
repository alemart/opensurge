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

    fun isEntityConsumed(entity)
    {
        return consumableEntityTracker.isConsumed(entity);
    }

    fun consumeEntity(entity)
    {
        consumableEntityTracker.persistentlyConsume(entity);
    }

    fun consumeEntityUntilLevelChange(entity)
    {
        consumableEntityTracker.transientlyConsume(entity);
    }
}

object "SurgeTheRabbit - Consumable Entity Tracker"
{
    /*

    All entities instantiated with Level.spawnEntity() are flagged consumed or
    not consumed. Initially, none are consumed. Any such entity may be flagged
    consumed. A consumable entity is an entity that will become consumed under
    some circumstances.

    Entities may be transiently or persistently consumed. Transiently consumed
    entities typically remain flagged until another level is loaded. Restarting
    the level doesn't affect the flag. On the other hand, persistently consumed
    entities remain consumed until the Game State is reset explicitly.

    Life Powerups are examples of consumable entities that are transiently
    consumed. Bonus Level Warps are examples of consumable entities that are
    persistently consumed.

    */

    watchedLevelFile = "";
    transientlyConsumedEntities = [];
    persistentlyConsumedEntities = [];

    state "main"
    {
        // clear transiently consumed entities on level change
        if(watchedLevelFile != Level.file) {
            watchedLevelFile = Level.file;
            transientlyConsumedEntities.clear();
        }
    }

    // transiently consume an entity
    fun transientlyConsume(entity)
    {
        _consume(entity, transientlyConsumedEntities);
    }

    // persistently consume an entity
    fun persistentlyConsume(entity)
    {
        _consume(entity, persistentlyConsumedEntities);
    }

    // check if an entity is consumed
    fun isConsumed(entity)
    {
        entityId = Level.entityId(entity);

        return transientlyConsumedEntities.indexOf(entityId) >= 0 ||
               persistentlyConsumedEntities.indexOf(entityId) >= 0;
    }

    // forcibly reset the state
    fun reset()
    {
        transientlyConsumedEntities.clear();
        persistentlyConsumedEntities.clear();
    }

    // internal logic for consuming an entity
    fun _consume(entity, list)
    {
        entityId = Level.entityId(entity);

        // sanity check: is it really an entity?
        if(String.isNullOrEmpty(entityId)) {
            Console.print("Can't consume entity: invalid ID");
            return;
        }

        // add entity to the list
        if(list.indexOf(entityId) < 0)
            list.push(entityId);
    }
}
