// -----------------------------------------------------------------------------
// File: end_of_demo.ss
// Description: end of demo script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine;
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Input;
using SurgeEngine.Level;
using SurgeEngine.Actor;
using SurgeEngine.Player;
using SurgeEngine.UI.Text;
using SurgeEngine.Video.Screen;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Audio.Music;
using SurgeEngine.Camera;
using SurgeEngine.Web;



// ----------------------------------------------------------------------------
// END OF DEMO SCENE: SETUP OBJECT
// ----------------------------------------------------------------------------

object "End of Demo"
{
    fader = spawn("Fader");
    text = spawn("End of Demo - Text");
    version = spawn("End of Demo - Version");
    music = spawn("End of Demo - Music");
    button = null;

    state "main"
    {
    }

    state "fading"
    {
        if(timeout(fader.fadeTime)) {
            Level.loadNext();
            state = "done";
        }
    }

    state "done"
    {
    }

    fun goToNextLevel()
    {
        fader.fadeOut();
        state = "fading";
    }

    fun onMusicEnded()
    {
        button = spawn("End of Demo - Button");
    }

    fun constructor()
    {
        fader.fadeTime = 0.5;
        fader.fadeIn();
    }
}



// ----------------------------------------------------------------------------
// MUSIC
// ----------------------------------------------------------------------------

object "End of Demo - Music"
{
    jingle = Music("musics/gameover.ogg");

    state "main"
    {
        jingle.play();
        state = "watching";
    }

    state "watching"
    {
        if(!jingle.playing || timeout(10.0)) {
            parent.onMusicEnded();
            state = "done";
        }
    }

    state "done"
    {
    }
}



// ----------------------------------------------------------------------------
// TEXTS
// ----------------------------------------------------------------------------

object "End of Demo - Version" is "private", "detached", "entity"
{
    transform = Transform();
    label = Text("End of Demo - Text");

    fun constructor()
    {
        transform.position = Vector2(348, 208);
        label.align = "right";
        label.text = "$ENDOFDEMO_VERSION";
    }
}

object "End of Demo - Text" is "private", "detached", "entity"
{
    transform = Transform();
    label = Text("End of Demo - Text");

    fun constructor()
    {
        transform.position = Vector2(Screen.width / 2, Screen.height / 2);
        label.align = "center";
        label.text = "$ENDOFDEMO_TEXT";
        label.maxWidth = Screen.width - 16;
    }
}



// ----------------------------------------------------------------------------
// CONTINUE BUTTON
// ----------------------------------------------------------------------------

object "End of Demo - Button" is "private", "detached", "entity"
{
    menu = spawn("Simple Menu")
           .addOption("continue", "$ENDOFDEMO_CONTINUE", Vector2.zero)
           .setIcon("End of Demo - Pointer")
           .setFontName("End of Demo - Text")
           .setHighlightColor("ffffff")
           .setPosition(Vector2(88, 208));

    fun onChooseMenuOption(optionId)
    {
        parent.goToNextLevel();
    }
}