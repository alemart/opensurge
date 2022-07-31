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
        if(!jingle.playing) {
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
        transform.position = Vector2(213, 120);
        label.align = "center";
        label.text = "$ENDOFDEMO_TEXT";
    }
}



// ----------------------------------------------------------------------------
// CONTINUE BUTTON
// ----------------------------------------------------------------------------

object "End of Demo - Button" is "private", "detached", "entity"
{
    transform = Transform();
    label = Text("End of Demo - Text");
    icon = spawn("End of Demo - Button - Blinking Icon");
    sfx = Sound("samples/select.wav");
    input = Input("default");

    state "main"
    {
        if(input.buttonPressed("fire1") || input.buttonPressed("fire3")) {
            sfx.play();
            parent.goToNextLevel();
            state = "done";
        }
    }

    state "done"
    {
    }

    fun constructor()
    {
        transform.position = Vector2(88, 208);
        label.align = "left";
        label.text = "$ENDOFDEMO_CONTINUE";
    }
}

object "End of Demo - Button - Blinking Icon" is "private", "detached", "entity"
{
    transform = Transform();
    label = Text("End of Demo - Text");
    seconds = 0.25;

    state "main"
    {
        label.visible = true;
        if(timeout(seconds))
            state = "alt";
    }

    state "alt"
    {
        label.visible = false;
        if(timeout(seconds))
            state = "main";
    }

    fun constructor()
    {
        transform.localPosition = Vector2(-10, 0);
        label.align = "left";
        label.text = ">";
    }
}