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
    buttonList = spawn("MainMenuButtonList")
    .withTitle("$MAINMENU_TITLE") // will use the appropriate translation
    .withButtons([
        "$MAINMENU_PLAY",
        "$MAINMENU_CREATE",
        "$MAINMENU_SHARE",
        "$MAINMENU_OPTIONS",
        "$MAINMENU_QUIT"
    ]);
    nextState = "";
    fadeTime = 0.5;
    shareURL = "http://opensurge2d.org/share";

    state "main"
    {
        if(input.buttonPressed("fire1") || input.buttonPressed("fire3"))
            buttonList.confirm();
        else if(input.buttonDown("up") || input.buttonDown("left"))
            buttonList.scrollDown();
        else if(input.buttonDown("down") || input.buttonDown("right"))
            buttonList.scrollUp();
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

    state "options"
    {
        Level.load("quests/options.qst");
    }

    state "quit"
    {
        Level.abort();
    }

    fun onMainMenuButton(buttonIndex)
    {
        if(buttonIndex == 0) {
            // play
            fadeTo("play");
        }
        else if(buttonIndex == 1) {
            // create
            Console.print("Coming soon!");
            fadeTo("restart");
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

    fun constructor()
    {
        fader.fadeIn(fadeTime);
    }
}

object "SurgeCircle" is "private", "entity"
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
    }
}

object "MenuCircle" is "private", "entity"
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

object "MainMenuButtonList"
{
    buttons = [];
    currentButtonIndex = 0;
    oldButtonIndex = 0;
    totalMoveTime = 0.3; // transition time, in seconds
    currentMoveTime = 0.0;
    buttonSpacing = Vector2(105, 80);
    basePosition = Vector2(Screen.width * 0.68, Screen.height / 2 - 9);
    transform = Transform();
    slide = Sound("samples/slide.wav");
    select = Sound("samples/select.wav");
    title = spawn("MenuTitle");

    state "main"
    {
    }

    state "moving"
    {
        if(currentMoveTime < totalMoveTime) {
            currentMoveTime += Time.delta;
            x = Math.smoothstep(buttonSpacing.x * oldButtonIndex, buttonSpacing.x * currentButtonIndex, currentMoveTime / totalMoveTime);
            y = Math.smoothstep(buttonSpacing.y * oldButtonIndex, buttonSpacing.y * currentButtonIndex, currentMoveTime / totalMoveTime);
            transform.localPosition = Vector2(-x, -y).plus(basePosition);
        }
        else {
            currentMoveTime = 0.0;
            transform.localPosition = buttonSpacing.scaledBy(-currentButtonIndex).plus(basePosition);
            oldButtonIndex = currentButtonIndex;
            state = "main";
        }
    }

    state "pressing"
    {
        if(buttons[currentButtonIndex].pressed) {
            parent.onMainMenuButton(currentButtonIndex);
            state = "disappearing";
        }
    }

    state "disappearing"
    {
        transform.move(2.5 * Screen.width * Time.delta, 0);
    }

    fun confirm()
    {
        if(state == "main") {
            buttons[currentButtonIndex].press();
            state = "pressing";
        }
    }

    fun scrollDown()
    {
        if(state == "main") {
            if(currentButtonIndex > 0) {
                // update indexes
                oldButtonIndex = currentButtonIndex;
                currentButtonIndex--;

                // focus on the proper button
                for(j = 0; j < buttons.length; j++)
                    buttons[j].blur();
                buttons[currentButtonIndex].focus();

                // move
                slide.play();
                state = "moving";
            }
        }
    }

    fun scrollUp()
    {
        if(state == "main") {
            if(currentButtonIndex < buttons.length - 1) {
                // update indexes
                oldButtonIndex = currentButtonIndex;
                currentButtonIndex++;

                // focus on the proper button
                for(j = 0; j < buttons.length; j++)
                    buttons[j].blur();
                buttons[currentButtonIndex].focus();

                // move
                slide.play();
                state = "moving";
            }
        }
    }

    fun withButtons(buttonArray)
    {
        for(j = 0; j < buttonArray.length; j++)
            buttons.push(spawnButton(buttonArray[j]));
        init();
        return this;
    }

    fun withTitle(text)
    {
        title.text = text;
        return this;
    }


    // ---------------------------------------------
    // internal stuff
    // ---------------------------------------------

    fun spawnButton(label)
    {
        btn = spawn("MenuButton");
        btn.text = label;
        btn.sound = select;
        return btn;
    }

    fun init()
    {
        buttons[currentButtonIndex].focus();
        for(j = 0; j < buttons.length; j++)
            buttons[j].transform.localPosition = buttonSpacing.scaledBy(j);
        title.transform.localPosition = buttonSpacing.scaledBy(-1);//.15);
        state = "moving";
    }
}