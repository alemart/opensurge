// -----------------------------------------------------------------------------
// File: thanks_for_playing.ss
// Description: setup script of the Thanks for Playing scene
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Level;
using SurgeEngine.UI.Text;
using SurgeEngine.Video.Screen;
using SurgeEngine.Platform;
using SurgeEngine.Prefs;
using SurgeEngine;
using SurgeTheRabbit;
using SurgeEngine.Camera;

object "Thanks for Playing" is "setup", "private", "detached", "entity"
{
    fader = spawn("Fader").setFadeTime(0.5);

    title = spawn("Thanks for Playing - Title")
            .setPosition(Vector2(Screen.width / 2, 1));

    text = spawn("Thanks for Playing - Text")
           .setPosition(Vector2(Screen.width / 2, 64));

    menu = spawn("Simple Menu Builder")
           .addOption("yes", "$THANKSFORPLAYING_YES",  Vector2.zero)
           .addOption("no",  "$THANKSFORPLAYING_NO", Vector2.down.scaledBy(20 * 1))
           .setBackOption("no")
           .setPosition(Vector2(Screen.width / 2, 192))
           .setIcon("Thanks for Playing - Pointer")
           .setHighlightColor("ffee11")
           .setFontName("Thanks for Playing - Text")
           .setAlignment("center")
           .setHighlightSound("samples/pause_highlight.wav")
           .setChooseSound("samples/pause_confirm.wav")
           .build();

    memorizedTime = spawn("Thanks for Playing - Memorized Time");

    state "main"
    {
    }

    state "waiting"
    {
        if(timeout(3.0))
            quit();
    }

    state "fading"
    {
        if(timeout(fader.fadeTime)) {
            memorizedTime.memorize();
            Level.loadNext();
        }
    }

    fun quit()
    {
        if(state == "fading")
            return;

        fader.fadeOut();
        state = "fading";
    }

    fun onChooseMenuOption(optionId)
    {
        if(optionId == "yes") {
            SurgeTheRabbit.share();
            state = "waiting";
        }
        else
            quit();
    }

    fun constructor()
    {
        // skip the screen?
        if(memorizedTime.isRecent()) {
            Level.loadNext();
            return;
        }

        fader.fadeIn();
    }
}

object "Thanks for Playing - Title" is "private", "detached", "entity"
{
    transform = Transform();
    label = Text("Thanks for Playing - Title");

    fun constructor()
    {
        label.text = "$THANKSFORPLAYING_TITLE";
        label.align = "center";
    }

    fun setPosition(position)
    {
        transform.position = position;
        return this;
    }
}

object "Thanks for Playing - Text" is "private", "detached", "entity"
{
    transform = Transform();
    label = Text("Thanks for Playing - Text");

    fun constructor()
    {
        label.text = "$THANKSFORPLAYING_TEXT";
        label.align = "center";
        label.maxWidth = Screen.width - 48;
    }

    fun setPosition(position)
    {
        transform.position = position;
        return this;
    }
}

// This object memorizes the time when the user sees the scene
object "Thanks for Playing - Memorized Time"
{
    key = generateKey("share-screen-time");
    secondsInADay = 86400;
    timeLimit = secondsInADay; // ask every day

    fun memorize()
    {
        Prefs[key] = Date.unixtime;
    }

    fun forget()
    {
        Prefs.delete(key);
    }

    fun isRecent()
    {
        currentTime = Date.unixtime;
        storedTime = Prefs.has(key) ? Number(Prefs[key]) : 0;
        deltaTime = Math.max(currentTime - storedTime, 0);

        return deltaTime < timeLimit;
    }

    fun generateKey(prefix)
    {
        return prefix + "-" + SurgeEngine.version;
    }
}