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
using SurgeEngine;
using SurgeTheRabbit;


object "Thanks for Playing" is "setup", "private", "detached", "entity"
{
    fader = spawn("Fader").setFadeTime(0.5);

    title = spawn("Thanks for Playing - Title");

    textA = spawn("Thanks for Playing - Text")
            .setPosition(Vector2(34, 48))
            .setMaxWidth(Screen.width - 34 * 2)
            .setText("$THANKSFORPLAYING_PARAGRAPH1");

    textB = spawn("Thanks for Playing - Text")
            .setPosition(Vector2(34, 87))
            .setMaxWidth(Screen.width - 34 * 2)
            .setText("$THANKSFORPLAYING_PARAGRAPH2");

    textC = spawn("Thanks for Playing - Text")
            .setPosition(Vector2(234, 128))
            .setMaxWidth(160)
            .setText("$THANKSFORPLAYING_PARAGRAPH3");

    menu = spawn("Simple Menu Builder")
           .addOption("yes", "$THANKSFORPLAYING_YES",  Vector2.zero)
           .addOption("no",  "$THANKSFORPLAYING_NO", Vector2.down.scaledBy(16 * 1))
           .setPosition(Vector2(250, 184))
           .setIcon("Thanks for Playing - Pointer")
           .setHighlightColor("ffee11")
           .setFontName("Thanks for Playing - Text")
           .setAlignment("left")
           .build();

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
        if(timeout(fader.fadeTime))
            Level.loadNext();
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
            SurgeTheRabbit.download();
            state = "waiting";
        }
        else
            quit();
    }

    fun constructor()
    {
        fader.fadeIn();

        if(SurgeEngine.mobile)
            Level.loadNext();
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
        label.maxWidth = Screen.width - 16;

        transform.position = Vector2(Screen.width / 2, -1);
    }
}

object "Thanks for Playing - Text" is "private", "detached", "entity"
{
    transform = Transform();
    label = Text("Thanks for Playing - Text");

    fun constructor()
    {
        label.text = "";
        label.align = "left";
    }

    fun setText(text)
    {
        label.text = text;
        return this;
    }

    fun setMaxWidth(maxWidth)
    {
        label.maxWidth = maxWidth;
        return this;
    }

    fun setPosition(position)
    {
        transform.position = position;
        return this;
    }
}