// -----------------------------------------------------------------------------
// File: create.ss
// Description: create screen - menu script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Level;
using SurgeEngine.Player;
using SurgeEngine.Input;
using SurgeEngine.UI.Text;
using SurgeEngine.Video.Screen;
using SurgeEngine.Camera;
using SurgeEngine.Web;

object "CreateMenu"
{
    //profiler = spawn("Profiler");
    input = Input(null);
    fader = spawn("Fader");
    circle = spawn("MenuCircle");
    text = spawn("CreateMenuText");
    title = spawn("MenuBuilder").withTitle("$CREATEMENU_TITLE").at(Screen.width / 2, 82).build();
    menu = spawn("MenuBuilder").at(Screen.width / 2, Screen.height - 36).withButtons(
        ["$CREATEMENU_PLAY", "$CREATEMENU_LEARN", "$CREATEMENU_BACK" ]
    ).withSpacing(211).withAxisAngle(0).build();
    learnURL = "http://opensurge2d.org";
    nextState = "";
    fadeTime = 0.5;

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
            fader.fadeOut(fadeTime);
            state = "fading";
        }
    }

    state "fading"
    {
        if(timeout(fadeTime))
            state = nextState;
    }

    state "play"
    {
        Level.load("quests/demo.qst");
    }

    state "restart"
    {
        Level.restart();
    }

    state "back"
    {
        Level.abort();
    }

    fun onMenuSelected(buttonIndex)
    {
        if(buttonIndex == 0) {
            // play
            fadeTo("play");
        }
        else if(buttonIndex == 1) {
            // learn
            Web.launchURL(learnURL);
            fadeTo("restart");
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

    fun resetPlayerData()
    {
        player = Player.active;
        player.lives = Player.initialLives;
        player.score = 0;
    }

    fun constructor()
    {
        resetPlayerData();
        fader.fadeIn(fadeTime);
    }
}

object "CreateMenuText" is "private", "entity"
{
    transform = Transform();
    text = Text("menu.bold");
    shadow = Text("menu.bold");

    fun constructor()
    {
        transform.position = Vector2(7, 55);
        text.maxWidth = Screen.width - 28;
        text.text = "$CREATEMENU_TEXT";
        shadow.maxWidth = text.maxWidth;
        shadow.text = "<color=000000>$CREATEMENU_TEXT</color>";
        shadow.offset = Vector2(0, 1);
        shadow.zindex = text.zindex - 0.1;
    }
}