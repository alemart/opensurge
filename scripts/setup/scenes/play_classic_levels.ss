// -----------------------------------------------------------------------------
// File: play_classic_levels.ss
// Description: setup script of the Play Classic Levels scene
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Input;
using SurgeEngine.Level;
using SurgeEngine.UI.Text;
using SurgeEngine.Video.Screen;
using SurgeEngine.Audio.Sound;



object "Play Classic Levels" is "setup"
{
    fader = spawn("Fader");

    text = spawn("Play Classic Levels - Text")
           .setText("$PLAYCLASSICLEVELS_TEXT")
           .setPosition(Vector2(Screen.width / 2, 128));

    menu = spawn("Simple Menu")
           .addOption("yes", "$PLAYCLASSICLEVELS_YES", Vector2.zero)
           .addOption("no",  "$PLAYCLASSICLEVELS_NO",  Vector2.down.scaledBy(18))
           .setIcon("End of Demo - Pointer")
           .setFontName("End of Demo - Text")
           .setAlignment("center")
           .setHighlightColor("ffffff")
           .setPosition(Vector2(Screen.width / 2, 160));

    state "main"
    {
    }

    state "load next level"
    {
        if(timeout(fader.fadeTime))
            Level.loadNext();
    }

    state "abort quest"
    {
        if(timeout(fader.fadeTime))
            Level.abort();
    }

    fun onChooseMenuOption(optionId)
    {
        fader.fadeOut();

        if(optionId == "yes")
            state = "load next level";
        else
            state = "abort quest";
    }

    fun constructor()
    {
        fader.fadeTime = 0.5;
        fader.fadeIn();
    }
}



object "Play Classic Levels - Text" is "private", "detached", "entity"
{
    transform = Transform();
    label = Text("End of Demo - Text");

    fun setPosition(position)
    {
        transform.position = position;
        return this;
    }

    fun setText(text)
    {
        label.text = text;
        return this;
    }

    fun constructor()
    {
        label.align = "center";
        label.maxWidth = Screen.width - 16;
    }
}