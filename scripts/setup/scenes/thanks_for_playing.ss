// -----------------------------------------------------------------------------
// File: play_classic_levels.ss
// Description: setup script of the Play Classic Levels scene
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Level;
using SurgeEngine.UI.Text;
using SurgeEngine.Video.Screen;

object "Thanks for Playing Scene" is "setup"
{
    text = spawn("Thanks for Playing Scene - Text");
    fader = spawn("Fader");

    state "main"
    {
        if(timeout(fader.fadeTime))
            state = "waiting";
    }

    state "waiting"
    {
        if(timeout(8.0)) {
            fader.fadeOut();
            state = "fading";
        }
    }

    state "fading"
    {
        if(timeout(fader.fadeTime))
            Level.loadNext();
    }

    fun constructor()
    {
        fader.fadeTime = 1.0;
        fader.fadeIn();
    }
}

object "Thanks for Playing Scene - Text" is "private", "detached", "entity"
{
    transform = Transform();
    label = Text("GoodNeighborsLarge");

    fun constructor()
    {
        label.text = "$THANKSFORPLAYING_TEXT";
        label.align = "center";
        label.maxWidth = Screen.width - 16;

        transform.position = Vector2(Screen.width / 2, 36);
    }
}