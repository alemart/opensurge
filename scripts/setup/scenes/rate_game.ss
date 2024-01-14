// -----------------------------------------------------------------------------
// File: rate_game.ss
// Description: "Rate the Game" scene script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine;
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Input;
using SurgeEngine.Input.Mouse;
using SurgeEngine.Input.MobileGamepad;
using SurgeEngine.Level;
using SurgeEngine.Actor;
using SurgeEngine.UI.Text;
using SurgeEngine.Video.Screen;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Platform;
using SurgeEngine.Prefs;
using SurgeTheRabbit;



// "Rate the Game" scene
object "Rate the Game" is "setup"
{
    fader = spawn("Fader");
    dialog = spawn("Rate the Game - Dialog");
    ratingTime = spawn("Rate the Game - Rating Time");

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




    fun constructor()
    {
        //ratingTime.forget(); // *** uncomment for testing ***

        // don't repeatedly ask the player to rate the game
        if(ratingTime.isRecent())
            Level.loadNext();

        // configure the fader
        fader.fadeTime = 0.25;
        fader.fadeIn();
    }

    fun goToNextLevel()
    {
        fader.fadeOut();
        state = "fading";
    }

    fun onChooseYes()
    {
        SurgeTheRabbit.rate();
        ratingTime.memorize();

        goToNextLevel();
    }

    fun onChooseNo()
    {
        goToNextLevel();
    }
}

// This object memorizes when the user has rated the game
object "Rate the Game - Rating Time"
{
    key = generateKey("rate-game-time");

    fun memorize()
    {
        Prefs[key] = Date.unixtime;
    }

    fun forget()
    {
        Prefs.delete(key);
    }

    fun isRecent()
    {
        currentTime = Date.unixtime;
        ratingTime = Prefs.has(key) ? Number(Prefs[key]) : 0;
        deltaTime = Math.max(currentTime - ratingTime, 0);

        return deltaTime < timeLimit();
    }

    fun timeLimit()
    {
        secondsInADay = 86400;
        daysInAYear = 365;
        oneYear = secondsInADay * daysInAYear;

        return oneYear; // ask once in a year (per key)
    }

    fun generateKey(prefix)
    {
        if(SurgeTheRabbit.isDevelopmentBuild())
            return prefix + "-" + SurgeEngine.version;
        else
            return prefix;
    }
}

// A graphic dialog
object "Rate the Game - Dialog" is "private", "detached", "entity"
{
    transform = Transform();
    dialogGraphic = Actor("Rate the Game - Dialog");
    title = Text("Rate the Game - Title");
    message = Text("Rate the Game - Text");
    menu = SurgeEngine.mobile ? spawn("Rate the Game - Mobile Menu") : spawn("Rate the Game - Desktop Menu");
    tween = spawn("Rate the Game - Dialog - Tween").setTransform(transform);

    fun onChooseYes()
    {
        tween.from(0).to(Screen.height).during(0.25).after(0.0);
        tween.start();

        parent.onChooseYes();
    }

    fun onChooseNo()
    {
        tween.from(0).to(Screen.height).during(0.25).after(0.0);
        tween.start();

        parent.onChooseNo();
    }

    fun constructor()
    {
        dialogGraphic.zindex = 0.1;

        title.offset = Vector2(Screen.width / 2, 35);
        title.align = "center";
        title.text = "<color=fb8e24>$RATEGAME_TITLE</color>";
        title.zindex = dialogGraphic.zindex + 0.1;

        message.offset = Vector2(Screen.width / 2, 68);
        message.align = "center";
        message.text = "<color=404040>$RATEGAME_MESSAGE</color>";
        message.maxWidth = 328;
        message.zindex = dialogGraphic.zindex + 0.1;

        transform.position = Vector2(0, Screen.height);
        tween.from(transform.position.y).to(0).during(0.33).after(0.25);
        tween.start();
    }
}

// Tweening
object "Rate the Game - Dialog - Tween"
{
    transform = null;
    startTime = 0.0;
    duration = 1.0;
    initialValue = 0;
    finalValue = 0;
    t = 0.0;

    state "main"
    {
        // sleep
    }

    state "ready"
    {
        if(timeout(startTime)) {
            t = 0.0;
            transform.position = Vector2(0, initialValue);
            state = "tweening";
        }
    }

    state "tweening"
    {
        t += Time.delta;

        currentValue = Math.smoothstep(initialValue, finalValue, t / duration);
        transform.translateBy(0, currentValue - transform.position.y);

        if(t >= duration)
            state = "done";
    }

    state "done"
    {
    }

    fun setTransform(t)
    {
        transform = t;
        return this;
    }
    
    fun during(seconds)
    {
        duration = seconds;
        return this;
    }

    fun after(seconds)
    {
        startTime = seconds;
        return this;
    }

    fun from(value)
    {
        initialValue = value;
        return this;
    }

    fun to(value)
    {
        finalValue = value;
        return this;
    }

    fun start()
    {
        if(transform !== null)
            state = "ready";
    }
}







//
// The following menu will be displayed on Desktop computers
//

object "Rate the Game - Desktop Menu" is "private", "detached", "entity"
{
    menu = spawn("Simple Menu Builder")
           .addOption("yes", "$RATEGAME_YES", Vector2.zero)
           .addOption("no",  "$RATEGAME_NO",  Vector2.down.scaledBy(38))
           .setIcon("Rate the Game - Desktop Menu - Cursor")
           .setFontName("Rate the Game - Text")
           .setAlignment("center")
           .setDefaultColor("404040")
           .setHighlightColor("fb8e24")
           .setPosition(Vector2(Screen.width / 2, 122))
           .build();

    input = Input("default");
    backButton = "fire4";

    state "main"
    {
        if(input.buttonPressed(backButton)) {
            menu.setHighlightedOption("no");
            menu.chooseHighlightedOption();
        }
    }

    fun onChooseMenuOption(optionId)
    {
        if(optionId == "yes")
            parent.onChooseYes();
        else
            parent.onChooseNo();
    }
}




//
// The following menu will be displayed on mobile devices
//

object "Rate the Game - Mobile Menu" is "private", "detached", "entity"
{
    primaryButton = spawn("Rate the Game - Mobile Menu - Button")
                    .setStyle("primary")
                    .setText("$RATEGAME_YES");

    secondaryButton = spawn("Rate the Game - Mobile Menu - Button")
                      .setStyle("secondary")
                      .setText("$RATEGAME_NO");

    input = Input("default");
    backButton = "fire4";

    state "main"
    {
        if(input.buttonPressed(backButton)) {
            onButtonClick(secondaryButton);
            state = "done";
        }
    }

    state "done"
    {
    }

    fun constructor()
    {
        primaryButton.position = Vector2(Screen.width / 2, 128);
        secondaryButton.position = Vector2(Screen.width / 2, 176);

        MobileGamepad.fadeOut();
    }

    fun destructor()
    {
        MobileGamepad.fadeIn();
    }

    fun onButtonClick(button)
    {
        primaryButton.disable();
        secondaryButton.disable();

        if(button == primaryButton)
            parent.onChooseYes();
        else if(button == secondaryButton)
            parent.onChooseNo();
        else
            Console.print("Clicked unknown button: " + button);
    }
}

object "Rate the Game - Mobile Menu - Button" is "private", "detached", "entity"
{
    actor = Actor("Rate the Game - Mobile Menu - Primary Button");
    label = Text("Rate the Game - Text");
    transform = Transform();
    style = "primary"; // "primary" or "secondary"
    textColor = "ffffff";
    text = "";
    unpressed = 0;
    pressed = 1;

    state "main"
    {
        actor.anim = unpressed;

        if(Mouse.buttonPressed("left")) {
            if(isOverlappingButton(Mouse.position)) {
                actor.anim = pressed;
                state = "pressed";
            }
        }
    }

    state "pressed"
    {
        if(Mouse.buttonDown("left")) {
            if(isOverlappingButton(Mouse.position))
                actor.anim = pressed;
            else
                actor.anim = unpressed;
        }
        else if(Mouse.buttonReleased("left")) {
            actor.anim = unpressed;
            state = "main";

            if(isOverlappingButton(Mouse.position))
                parent.onButtonClick(this);
        }
        else {
            actor.anim = unpressed;
            state = "main";
        }
    }

    state "disabled"
    {
        actor.anim = unpressed;
    }

    fun disable()
    {
        state = "disabled";
    }

    fun setStyle(newStyle)
    {
        if(newStyle == "primary") {
            actor = Actor("Rate the Game - Mobile Menu - Primary Button");
            textColor = "ffffff";
        }
        else if(newStyle == "secondary") {
            actor = Actor("Rate the Game - Mobile Menu - Secondary Button");
            textColor = "404040";
        }

        actor.anim = unpressed;
        this.setText(text);
        return this;
    }

    fun setText(newText)
    {
        text = newText;

        label.text = "<color=" + textColor + ">" + text + "</color>";
        label.zindex = actor.zindex + 0.1;
        label.offset = actor.actionOffset;
        label.align = "center";

        return this;
    }

    fun set_position(position)
    {
        transform.localPosition = position;
    }

    fun get_position()
    {
        return transform.localPosition;
    }

    fun isOverlappingButton(target)
    {
        top = transform.position.y - actor.hotSpot.y;
        dy = target.y - top;

        // test collision on the y-axis
        if(0 <= dy && dy < actor.height) {
            left = transform.position.x - actor.hotSpot.x;
            dx = target.x - left;

            // test collision on the x-axis
            if(0 <= dx && dx < actor.width)
                return true;
        }

        return false;
    }
}