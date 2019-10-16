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
using SurgeEngine.UI.Text;
using SurgeEngine.Video.Screen;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Camera;
using SurgeEngine.Lang;
using SurgeEngine.Web;

object "MainMenu"
{
    input = Player.active.input;
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
    camera = spawn("MainMenuCameraEffect")
        .startingAt(Screen.width * 0.7, Screen.height * 0.8)
        .during(0.7);
    shareURL = "http://opensurge2d.org/share?lang=" + Lang["LANG_ID"] + "&v=" + SurgeEngine.version;
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
        fader.fadeIn();
    }
}

object "SurgeCircle" is "private", "detached", "entity", "awake"
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

object "MenuCircle" is "private", "detached", "entity", "awake"
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
        text[0].zindex = text[1].zindex = 1.0;
        if(SurgeEngine.version.indexOf("dev") >= 0) {
            text[0].align = "right";
            text[0].text = "$MAINMENU_DEVBUILD";
            text[1].align = "right";
            text[1].text = SurgeEngine.version;
            text[1].offset = Vector2(0, 12);
        }
        else {
            text[0].align = "right";
            text[0].text = "$MAINMENU_VERSION " + SurgeEngine.version;
            text[1].visible = false;
        }
    }
}

object "MainMenuCameraEffect"
{
    initialPosition = Vector2.zero;
    finalPosition = Vector2(Screen.width / 2, Screen.height / 2);
    totalTime = 0.5;
    t = 0.0;

    state "main"
    {
        Camera.position = initialPosition;
        state = "move";
    }

    state "move"
    {
        Camera.position = interpolate(initialPosition, finalPosition, t);
        t += Time.delta / totalTime;
        if(t >= 1.0) {
            Camera.position = finalPosition;
            state = "done";
        }
    }

    state "done"
    {
    }

    fun interpolate(a, b, t)
    {
        // interpolate by t
        //return a.plus(b.minus(a).scaledBy(t));
        x = Math.smoothstep(a.x, b.x, t);
        y = Math.smoothstep(a.y, b.y, t);
        return Vector2(x, y);
    }

    fun startingAt(x, y)
    {
        initialPosition = Vector2(x, y);
        return this;
    }

    fun finishingAt(x, y)
    {
        finalPosition = Vector2(x, y);
        return this;
    }

    fun during(t)
    {
        totalTime = Math.max(0, t);
        return this;
    }
}