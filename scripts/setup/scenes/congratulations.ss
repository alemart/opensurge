// -----------------------------------------------------------------------------
// File: congratulations.ss
// Description: congratulations screen
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine;
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Level;
using SurgeEngine.Player;
using SurgeEngine.UI.Text;
using SurgeEngine.Video.Screen;
using SurgeEngine.Camera;
using SurgeEngine.Lang;
using SurgeEngine.Web;

object "Congratulations"
{
    input = Player.active.input;
    fader = spawn("Fader");
    circle = spawn("MenuCircle");
    text = spawn("CongratulationsText");
    title = spawn("CongratulationsTitle");
    menu = spawn("MenuBuilder").at(Screen.width / 2, Screen.height - 36).withButtons(
        ["$CONGRATULATIONS_SHARE", "$CONGRATULATIONS_DONATE", "$CONGRATULATIONS_BACK" ]
    ).withSpacing(211).withAxisAngle(0).build();
    nextState = "";

    state "main"
    {
        if(input.buttonPressed("fire1") || input.buttonPressed("fire3"))
            menu.select();
        else if(input.buttonDown("up") || input.buttonDown("left"))
            menu.movePrevious();
        else if(input.buttonDown("down") || input.buttonDown("right"))
            menu.moveNext();
    }

    state "waitToFade"
    {
        if(timeout(0.5)) {
            fader.fadeOut();
            state = "fading";
        }
    }

    state "fading"
    {
        if(timeout(fader.fadeTime))
            state = nextState;
    }

    state "back"
    {
        Level.abort();
    }

    fun onMenuSelected(buttonIndex)
    {
        if(buttonIndex == 0) {
            // share
            Web.launchURL(shareURL());
            fadeTo("back");
        }
        else if(buttonIndex == 1) {
            // donate
            Web.launchURL(donateURL());
            fadeTo("back");
        }
        else if(buttonIndex == 2) {
            // back
            fadeTo("back");
        }
    }

    fun fadeTo(newState)
    {
        nextState = newState;
        state = "waitToFade";
    }

    fun shareURL()
    {
        return "http://opensurge2d.org/share?v=" + SurgeEngine.version + "&lang=" + Lang["LANG_ID"];
    }

    fun donateURL()
    {
        return "http://opensurge2d.org/contribute?v=" + SurgeEngine.version + "&lang=" + Lang["LANG_ID"];
    }
}

object "CongratulationsTitle" is "private", "entity"
{
    transform = Transform();
    text = Text("MenuButton");

    fun constructor()
    {
        transform.position = Vector2(Screen.width / 2, 18);
        text.align = "center";
        text.text = "$CONGRATULATIONS_TITLE";
    }
}

object "CongratulationsText" is "private", "entity"
{
    transform = Transform();
    text = Text("MenuBold");
    shadow = Text("MenuBold");

    fun constructor()
    {
        transform.position = Vector2(7, 55);
        text.maxWidth = Screen.width - 28;
        text.text = "$CONGRATULATIONS_TEXT";
        shadow.maxWidth = text.maxWidth;
        shadow.text = "<color=000000>$CONGRATULATIONS_TEXT</color>";
        shadow.offset = Vector2(0, 1);
        shadow.zindex = text.zindex - 0.1;
    }
}