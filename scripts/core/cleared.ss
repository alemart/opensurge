// -----------------------------------------------------------------------------
// File: cleared.ss
// Description: Level Cleared animation
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Transform;
using SurgeEngine.Player;
using SurgeEngine.Vector2;
using SurgeEngine.Video;
using SurgeEngine.Level;
using SurgeEngine.Audio.Music;
using SurgeEngine.Audio.Sound;
using SurgeEngine.UI.Text;

object "LevelClearedAnimation" is "entity", "awake", "detached", "private"
{
    title = [
        spawn("LevelClearedAnimation.Title").init(0),
        spawn("LevelClearedAnimation.Title").init(1)
    ];
    counter = [
        spawn("LevelClearedAnimation.Counter").init("score"),
        spawn("LevelClearedAnimation.Counter").init("power"),
        spawn("LevelClearedAnimation.Counter").init("time")
    ];
    jingle = [
        Music("musics/winning.ogg"),
        Music("musics/winning_plus.ogg")
    ];

    state "main"
    {
        if(Level.cleared) {
            //Level.music.stop();
            state = "cleared";
        }
    }

    state "cleared"
    {
        Level.music.volume -= Time.delta;
        if(timeout(1.5)) {
            Level.music.stop();
            if(Player.active.collectibles >= 100) {
                jingle[1].play();
                state = "jingle+ warmup";
            }
            else {
                jingle[0].play();
                state = "jingle warmup";
            }
        }
    }

    state "jingle warmup"
    {
        if(timeout(0.5))
            state = "show title";
    }

    state "jingle+ warmup"
    {
        if(timeout(2.0))
            state = "show title";
    }

    state "show title"
    {
        title[0].appear();
        if(timeout(0.5)) {
            title[1].appear();
            state = "show counters";
        }
    }

    state "show counters"
    {
        if(timeout(0.15)) {
            counter[0].appear();
            counter[1].appear();
            counter[2].appear();
            state = "wait";
        }
    }

    state "wait"
    {
    }

    state "finish"
    {
        if(!jingle[0].playing && !jingle[1].playing) {
            title[0].disappear();
            title[1].disappear();
            counter[0].disappear();
            counter[1].disappear();
            counter[2].disappear();
            state = "fadeout";
        }
    }

    state "fadeout"
    {
        // TODO: fade out
        if(timeout(1.0)) {
            state = "done";
        }
    }

    state "done"
    {
        if(timeout(2.0))
            Level.finish();
    }

    fun finish()
    {
        state = "finish";
    }
}

object "LevelClearedAnimation.Title" is "entity", "awake", "detached", "private"
{
    transform = Transform();
    text = Text("GoodNeighborsLarge");
    appearSpeed = 3 * Video.screenWidth; // in px/s

    state "main"
    {
    }

    state "appearing"
    {
        transform.move(appearSpeed * Time.delta, 0);
        if(appearSpeed > 0) {
            if(transform.localPosition.x >= Video.screenWidth / 2) {
                transform.localPosition = Vector2(Video.screenWidth / 2, transform.localPosition.y);
                state = "done";
            }
        }
        else {
            if(transform.localPosition.x <= Video.screenWidth / 2) {
                transform.localPosition = Vector2(Video.screenWidth / 2, transform.localPosition.y);
                state = "done";
            }
        }
    }

    state "disappearing"
    {
        transform.move(-appearSpeed * Time.delta, 0);
    }

    state "done"
    {
    }

    fun appear()
    {
        if(state == "main")
            state = "appearing";
    }

    fun disappear()
    {
        state = "disappearing";
    }

    fun init(lineId)
    {
        // configure the text according to lineId
        if(lineId == 0) {
            text.text = "$CLEARED_LINE1";
            transform.localPosition = Vector2(3 * Video.screenWidth / 2, 48);
            appearSpeed *= -1.0;
        }
        else if(lineId == 1) {
            text.text = "$CLEARED_LINE2 " + Level.act;
            transform.localPosition = Vector2(-Video.screenWidth / 2, 76);
            appearSpeed *= 1.0;
        }
        else
            Application.crash("Invalid parameter for " + this.__name + ".init()");

        // return itself
        return this;
    }

    fun constructor()
    {
        text.align = "center";
    }
}

object "LevelClearedAnimation.Counter" is "entity", "awake", "detached", "private"
{
    transform = Transform();
    text = Text("GoodNeighbors");
    value = Text("GoodNeighbors");
    counter = null;
    appearSpeed = 3 * Video.screenWidth; // in px/s
    isMaster = false;
    countfx = null;
    donefx = null;

    state "main"
    {
    }

    state "appearing"
    {
        transform.move(appearSpeed * Time.delta, 0);
        if(transform.localPosition.x >= Video.screenWidth / 2) {
            transform.localPosition = Vector2(Video.screenWidth / 2, transform.localPosition.y);
            state = "wait";
        }
    }

    state "disappearing"
    {
        transform.move(-appearSpeed * Time.delta, 0);
    }

    state "wait"
    {
        if(timeout(3.5)) {
            state = "count";
        }
    }

    state "count"
    {
        if(timeout(0.03)) {
            value.text = counter.count(100);
            state = "repeat";
        }
    }

    state "repeat"
    {
        if(counter.finished) {
            if(isMaster)
                donefx.play();
            state = "finish";
        }
        else {
            if(isMaster)
                countfx.play();
            state = "count";
        }
    }

    state "finish"
    {
        if(isMaster) {
            if(timeout(2.0)) {
                parent.finish();
                state = "done";
            }
        }
        else
            state = "done";
    }

    state "done"
    {
    }

    fun appear()
    {
        if(state == "main") {
            value.text = counter.activate();
            state = "appearing";
        }
    }

    fun disappear()
    {
        state = "disappearing";
    }

    fun init(counterName)
    {
        if(counterName == "score") {
            text.text = "<color=ffee55>$CLEARED_SCORE</color>";
            counter = spawn("LevelClearedAnimation.Counter.Score");
            transform.localPosition = Vector2(-0.5 * Video.screenWidth, 112);
            appearSpeed *= 1.0;
            isMaster = true;
            countfx = Sound("samples/collectible_count.wav");
            donefx = Sound("samples/cash.wav");
        }
        else if(counterName == "time") {
            text.text = "<color=ffee55>$CLEARED_TIME</color>";
            counter = spawn("LevelClearedAnimation.Counter.Time");
            transform.localPosition = Vector2(-1.0 * Video.screenWidth, 128);
            appearSpeed *= 1.25;
        }
        else if(counterName == "power") {
            text.text = "<color=ffee55>$CLEARED_POWER</color>";
            counter = spawn("LevelClearedAnimation.Counter.Power");
            transform.localPosition = Vector2(-1.5 * Video.screenWidth, 144);
            appearSpeed *= 1.5;
        }
        else
            Application.crash("Invalid parameter for " + this.__name + ".init()");

        // return itself
        return this;
    }

    fun constructor()
    {
        text.align = "left";
        value.align = "right";
        text.zindex = value.zindex = 1000;
        text.transform.localPosition = Vector2(Video.screenWidth * -0.23, 0);
        value.transform.localPosition = Vector2(Video.screenWidth * 0.23, 0);
    }
}

// -----------------------------------------------------------------------------
// The objects below implement the counting logic
// -----------------------------------------------------------------------------

// Logic for counting the score
object "LevelClearedAnimation.Counter.Score"
{
    score = -1;
    powerBonus = -1;
    timeBonus = -1;
    seconds = 0;

    state "main"
    {
        seconds += Time.delta;
    }

    fun count(step)
    {
        // update counter
        deltaPower = Math.min(powerBonus, step);
        deltaTime = Math.min(timeBonus, step);
        powerBonus -= deltaPower;
        timeBonus -= deltaTime;
        score += deltaPower + deltaTime;

        // update player score
        Player.active.score = score;

        // done
        return score;
    }

    fun activate()
    {
        score = Player.active.score;
        powerBonus = Player.active.collectibles * 100;
        timeBonus = Math.ceil(10 * Math.max(0, 600 - seconds));
        return score;
    }

    fun get_finished()
    {
        return powerBonus + timeBonus == 0;
    }
}

// Logic for counting the Power Bonus
object "LevelClearedAnimation.Counter.Power"
{
    powerBonus = -1;

    fun count(step)
    {
        powerBonus -= Math.min(powerBonus, step);
        return powerBonus;
    }

    fun activate()
    {
        powerBonus = Player.active.collectibles * 100;
        return powerBonus;
    }

    fun get_finished()
    {
        return powerBonus == 0;
    }
}

// Logic for counting the Time Bonus
object "LevelClearedAnimation.Counter.Time"
{
    timeBonus = -1;
    seconds = 0;

    state "main"
    {
        seconds += Time.delta;
    }
    
    fun count(step)
    {
        timeBonus -= Math.min(timeBonus, step);
        return timeBonus;
    }

    fun activate()
    {
        timeBonus = Math.ceil(10 * Math.max(0, 600 - seconds));
        return timeBonus;
    }

    fun get_finished()
    {
        return timeBonus == 0;
    }
}