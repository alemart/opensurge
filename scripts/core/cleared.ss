// -----------------------------------------------------------------------------
// File: cleared.ss
// Description: default Level Cleared Animation
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Player;
using SurgeEngine.Level;
using SurgeEngine.Input;
using SurgeEngine.UI.Text;
using SurgeEngine.Audio.Music;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Video.Screen;
using SurgeEngine.Input.MobileGamepad;
using SurgeEngine.Events.Event;

/*

The default animation that is played when the level is cleared.

Public properties:
- onStart: event object. Triggered when the animation starts playing.
- onFinish: event object. Triggered when the animation finishes playing.
- wantJinglePlus: boolean. Play a different jingle when the number of
                           collectibles is greater than or equal to 100.

Tip: call Level.undoClear() in a onFinish event to restore normal gameplay
     after the level cleared animation finishes playing.

*/
object "Default Cleared Animation" is "entity", "awake", "detached", "private"
{
    public onStart = Event();
    public onFinish = Event();
    public wantJinglePlus = true;

    title = [
        spawn("DefaultClearedAnimation.Title").init(0),
        spawn("DefaultClearedAnimation.Title").init(1)
    ];
    counter = [
        spawn("DefaultClearedAnimation.Counter").init("score"),
        spawn("DefaultClearedAnimation.Counter").init("power"),
        spawn("DefaultClearedAnimation.Counter").init("time")
    ];
    jingle = [
        Music("musics/winning.ogg"),
        Music("musics/winning_plus.ogg")
    ];
    fader = spawn("Fader");
    previousVolume = 1.0;

    state "main"
    {
        if(Level.cleared) {
            //Level.music.stop();
            previousVolume = Level.music.volume;
            onStart.call();
            state = "cleared";
        }
    }

    state "cleared"
    {
        Level.music.volume -= Time.delta;
        if(timeout(1.5)) {
            Level.music.stop();
            if(wantJinglePlus && Player.active.collectibles >= 100) {
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
        if((!jingle[0].playing && !jingle[1].playing) || timeout(25.0)) {
            title[0].disappear();
            title[1].disappear();
            counter[0].disappear();
            counter[1].disappear();
            counter[2].disappear();

            onFinish.call();

            if(Level.cleared)
                state = "fadeout";
            else
                state = "restore"; // Level.undoClear() was called by some script,
                                   // possibly during the onFinish event call
        }
    }

    state "fadeout"
    {
        MobileGamepad.fadeOut();
        if(timeout(3.0)) {
            fader.fadeOut();
            state = "done";
        }
    }

    state "done"
    {
        if(timeout(1.0)) {
            MobileGamepad.fadeIn();
            Level.loadNext();
        }
    }

    state "restore"
    {
        Level.music.volume = previousVolume;
        Level.music.play();
        state = "main";
    }

    fun finish()
    {
        state = "finish";
    }
}

object "DefaultClearedAnimation.Title" is "entity", "awake", "detached", "private"
{
    transform = Transform();
    text = Text("HUD Large");
    initialPosition = Vector2.zero;
    appearSpeed = 3 * Screen.width; // in px/s

    state "main"
    {
    }

    state "appearing"
    {
        transform.translateBy(appearSpeed * Time.delta, 0);
        if(appearSpeed > 0) {
            if(transform.localPosition.x >= Screen.width / 2) {
                transform.localPosition = Vector2(Screen.width / 2, transform.localPosition.y);
                state = "done";
            }
        }
        else {
            if(transform.localPosition.x <= Screen.width / 2) {
                transform.localPosition = Vector2(Screen.width / 2, transform.localPosition.y);
                state = "done";
            }
        }
    }

    state "disappearing"
    {
        transform.translateBy(-appearSpeed * Time.delta, 0);
    }

    state "done"
    {
    }

    fun appear()
    {
        if(state == "main" || state == "disappearing") {
            transform.localPosition = initialPosition;
            refreshText();
            state = "appearing";
        }
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
            initialPosition = Vector2(3 * Screen.width / 2, 48);
            appearSpeed *= -1.0;
        }
        else if(lineId == 1) {
            text.text = "$CLEARED_LINE2";
            initialPosition = Vector2(-Screen.width / 2, 76);
            appearSpeed *= 1.0;
        }
        else
            Application.crash("Invalid parameter for " + this.__name + ".init()");

        // hide
        transform.localPosition = initialPosition;

        // return itself
        return this;
    }

    fun refreshText()
    {
        // Level.act may have been changed
        tmp = text.text;
        text.text = "";
        text.text = tmp;
    }

    fun constructor()
    {
        text.align = "center";
        text.zindex = 1000;
    }
}

object "DefaultClearedAnimation.Counter" is "entity", "awake", "detached", "private"
{
    transform = Transform();
    text = Text("HUD");
    value = Text("HUD");
    input = Input("default");
    initialPosition = Vector2.zero;
    counter = null;
    appearSpeed = 3 * Screen.width; // in px/s
    isMaster = false;
    countfx = null;
    donefx = null;

    state "main"
    {
    }

    state "appearing"
    {
        transform.translateBy(appearSpeed * Time.delta, 0);
        if(transform.localPosition.x >= Screen.width / 2) {
            transform.localPosition = Vector2(Screen.width / 2, transform.localPosition.y);
            state = "wait";
        }
    }

    state "disappearing"
    {
        transform.translateBy(-appearSpeed * Time.delta, 0);
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
            // accelerate the counter by holding FIRE1
            multiplier = input.buttonDown("fire1") ? 4 : 1;

            // count
            value.text = counter.count(100 * multiplier);
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
        if(state == "main" || state == "disappearing") {
            transform.localPosition = initialPosition;
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
            text.text = "<color=ffee11>$CLEARED_SCORE</color>";
            counter = spawn("DefaultClearedAnimation.Counter.Score");
            initialPosition = Vector2(-0.5 * Screen.width, 112);
            appearSpeed *= 1.0;
            isMaster = true;
            countfx = Sound("samples/collectible_count.wav");
            donefx = Sound("samples/cash.wav");
        }
        else if(counterName == "time") {
            text.text = "<color=ffee11>$CLEARED_TIME</color>";
            counter = spawn("DefaultClearedAnimation.Counter.Time");
            initialPosition = Vector2(-1.0 * Screen.width, 128);
            appearSpeed *= 1.25;
        }
        else if(counterName == "power") {
            text.text = "<color=ffee11>$CLEARED_POWER</color>";
            counter = spawn("DefaultClearedAnimation.Counter.Power");
            initialPosition = Vector2(-1.5 * Screen.width, 144);
            appearSpeed *= 1.5;
        }
        else
            Application.crash("Invalid parameter for " + this.__name + ".init()");

        // hide
        transform.localPosition = initialPosition;

        // return itself
        return this;
    }

    fun constructor()
    {
        text.align = "left";
        value.align = "right";
        text.zindex = value.zindex = 1000;
        text.offset = Vector2(Screen.width * -0.23, 0);
        value.offset = Vector2(Screen.width * 0.23, 0);
    }
}

// -----------------------------------------------------------------------------
// The objects below implement the counting logic
// -----------------------------------------------------------------------------

// Logic for counting the score
object "DefaultClearedAnimation.Counter.Score"
{
    score = -1;
    powerBonus = -1;
    timeBonus = -1;

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
        timeBonus = Math.ceil(10 * Math.max(0, 600 - Level.time));
        return score;
    }

    fun get_finished()
    {
        return powerBonus + timeBonus == 0;
    }
}

// Logic for counting the Power Bonus
object "DefaultClearedAnimation.Counter.Power"
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
object "DefaultClearedAnimation.Counter.Time"
{
    timeBonus = -1;

    fun count(step)
    {
        timeBonus -= Math.min(timeBonus, step);
        return timeBonus;
    }

    fun activate()
    {
        timeBonus = Math.ceil(10 * Math.max(0, 600 - Level.time));
        return timeBonus;
    }

    fun get_finished()
    {
        return timeBonus == 0;
    }
}