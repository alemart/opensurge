// -----------------------------------------------------------------------------
// File: mainmenu.ss
// Description: main menu script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine;
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Level;
using SurgeEngine.Actor;
using SurgeEngine.Player;
using SurgeEngine.Input;
using SurgeEngine.UI.Text;
using SurgeEngine.Video.Screen;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Web;

object "MainMenu"
{
    //profiler = spawn("Profiler");
    input = Input(null);
    fader = spawn("Fader");
    cool = spawn("SurgeCool");
    circle = spawn("SurgeCircle");
    circle2 = spawn("MenuCircle");
    version = spawn("MainMenuGameVersion");
    menu = spawn("MenuBuilder")
        .withTitle("$MAINMENU_TITLE")
        .withButtons([
            "$MAINMENU_PLAY",
            "$MAINMENU_CREATE",
            "$MAINMENU_SHARE",
            "$MAINMENU_OPTIONS",
            "$MAINMENU_QUIT"
        ])
        .withSpacing(132)
        .withAxisAngle(37.3)
        .at(
            Screen.width * 0.68,
            Screen.height / 2 - 9
        )
        .build();
    shareURL = "http://opensurge2d.org/share";
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
        Level.load("quests/default.qst");
    }

    state "restart"
    {
        Level.restart();
    }

    state "create"
    {
        Level.load("quests/create.qst");
    }

    state "options"
    {
        Level.load("quests/options.qst");
    }

    state "quit"
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
            // create
            fadeTo("create");
        }
        else if(buttonIndex == 2) {
            // share
            Web.launchURL(shareURL);
            fadeTo("restart");
        }
        else if(buttonIndex == 3) {
            // options
            fadeTo("options");
        }
        else if(buttonIndex == 4) {
            // quit
            fadeTo("quit");
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

object "SurgeCircle" is "private", "entity", "awake"
{
    transform = Transform();
    actor = Actor("SurgeCircle");
    angularSpeed = -30.0; // degrees / second

    state "main"
    {
        transform.rotate(angularSpeed * Time.delta);
    }

    fun constructor()
    {
        transform.position = Vector2(-12, Screen.height + 12);
        actor.zindex = 0;
    }
}

object "MenuCircle" is "private", "entity", "awake"
{
    transform = Transform();
    actor = Actor("MenuCircle");
    angularSpeed = -15.0; // degrees / second

    state "main"
    {
        transform.rotate(angularSpeed * Time.delta);
    }

    fun constructor()
    {
        transform.position = Vector2(-16, Screen.height + 16);
        transform.angle = -30;
        actor.zindex = 0;
    }
}

object "SurgeCool" is "private", "detached", "entity"
{
    transform = Transform();
    actor = Actor("SurgeCool");
    totalTime = 1.5; // appearance time, in seconds
    currentTime = 0.0;

    state "main"
    {
        if(currentTime == 0 || transform.position.y > Screen.height) {
            y = Math.smoothstep(Screen.height + actor.height, Screen.height, currentTime / totalTime);
            transform.position = Vector2(64, y);
            currentTime += Time.delta;
        }
        else {
            transform.position = Vector2(64, Screen.height);
            state = "done";
        }
    }

    state "done"
    {
    }
}

object "MainMenuGameVersion" is "private", "detached", "entity"
{
    transform = Transform();
    text = [ Text("menu.small"), Text("menu.small") ];

    state "main"
    {
    }

    fun constructor()
    {
        transform.position = Vector2(Screen.width - 4, 4);
        if(SurgeEngine.version.indexOf("dev") >= 0) {
            text[0].align = "right";
            text[0].text = "Development version";
            text[1].align = "right";
            text[1].text = SurgeEngine.version;
            text[1].offset = Vector2(0, 12);
        }
        else {
            text[0].align = "right";
            text[0].text = "ver. " + SurgeEngine.version;
            text[1].visible = false;
        }
    }
}