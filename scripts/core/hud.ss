// -----------------------------------------------------------------------------
// File: hud.ss
// Description: the default Heads-Up Display for Open Surge
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine;
using SurgeEngine.Transform;
using SurgeEngine.Player;
using SurgeEngine.Vector2;
using SurgeEngine.Actor;
using SurgeEngine.Level;
using SurgeEngine.UI.Text;
using SurgeEngine.Video.Screen;
using SurgeEngine.Input.Mouse;

object "Default HUD" is "entity", "detached", "awake", "private"
{
    public readonly transform = Transform();
    score = spawn("DefaultHUD.Score");
    timer = spawn("DefaultHUD.Time");
    collectibles = spawn("DefaultHUD.Collectibles");
    lives = spawn("DefaultHUD.Lives");
    powerups = spawn("DefaultHUD.Powerups");
    pause = spawn("DefaultHUD.PauseButton");

    state "main"
    {
        transform.position = Vector2(16, 8);
        state = "active";
    }

    state "active"
    {
        if(Level.cleared) {
            timer.block();
            state = "cleared";
        }
    }

    state "cleared"
    {
        //transform.translateBy(-Screen.width * Time.delta, 0);
    }

    // base zindex for the components of the HUD
    fun get_zindex()
    {
        return 1000.0;
    }
}

object "DefaultHUD.Score" is "entity", "detached", "awake", "private"
{
    transform = Transform();
    label = Text("HUD");
    value = Text("HUD");

    state "main"
    {
        transform.localPosition = Vector2.zero;
        label.zindex = value.zindex = parent.zindex;
        label.offset = Vector2(0, 0);
        value.offset = Vector2(56, 0);

        state = "active";
    }

    state "active"
    {
        label.text = "<color=ffee11>$HUD_SCORE</color>";
        value.text = Player.active.score;
    }
}

object "DefaultHUD.Time" is "entity", "detached", "awake", "private"
{
    transform = Transform();
    label = Text("HUD");
    value = Text("HUD");

    state "main"
    {
        transform.localPosition = Vector2(0, 16);
        label.zindex = value.zindex = parent.zindex;
        label.offset = Vector2(0, 0);
        value.offset = Vector2(56, 0);

        state = "active";
    }

    state "active"
    {
        time = Level.time;
        if(time < 0.1) // apparently blocked on level start
            time = 0.0;

        seconds = Math.floor(time);
        minutes = Math.floor(seconds / 60);
        sec = Math.mod(seconds, 60);
        dsec = Math.floor((time - seconds) * 100);

        value.text = minutes + "' " + pad(sec) + "\" " + pad(dsec);
        label.text = "<color=ffee11>$HUD_TIME</color>";
    }

    state "blocked"
    {
        // don't change the labels
    }

    fun pad(x)
    {
        if(x < 10)
            return "0" + x;
        else
            return x;
    }

    fun block()
    {
        state = "blocked";
    }
}

object "DefaultHUD.Collectibles" is "entity", "detached", "awake", "private"
{
    transform = Transform();
    label = Text("HUD");
    value = Text("HUD");
    blinkTime = 0.35;

    state "main"
    {
        transform.localPosition = Vector2(0, 32);
        label.zindex = value.zindex = parent.zindex;
        label.offset = Vector2(0, 0);
        value.offset = Vector2(56, 0);

        state = "active";
    }

    state "active"
    {
        label.text = "<color=ffee11>$HUD_POWER</color>";
        value.text = Player.active.collectibles;
        if(Player.active.collectibles == 0) {
            if(timeout(blinkTime))
                state = "blink";
        }
    }

    state "blink"
    {
        label.text = "<color=ff0055>$HUD_POWER</color>";
        value.text = Player.active.collectibles;
        if(timeout(blinkTime))
            state = "main";
    }
}

object "DefaultHUD.Lives" is "entity", "detached", "awake", "private"
{
    transform = Transform();
    value = Text("HUD");
    icon = null;
    currentPlayer = null;

    state "main"
    {
        value.zindex = parent.zindex;

        transform.localPosition = Vector2(0, Screen.height - 29);
        /*if(SurgeEngine.mobileMode)
            transform.position = Vector2(Screen.width * 0.87, parent.transform.position.y);*/

        state = "active";
    }

    state "active"
    {
        value.text = Player.active.lives;
        if(Player.active.name !== currentPlayer) {
            currentPlayer = Player.active.name;
            updateIcon(currentPlayer);
        }
    }

    fun updateIcon(playerName)
    {
        // destroy previous icon
        if(icon != null)
            icon.destroy();

        // create new icon
        icon = Actor("Life Icon " + playerName);
        if(!icon.animation.exists) {
            icon.destroy();
            icon = Actor("Life Icon");
        }

        // adjust icon
        icon.offset = Vector2(9, 4);
        icon.zindex = value.zindex;
        value.offset = Vector2(icon.width, 0);
    }
}

object "DefaultHUD.Powerups" is "entity", "detached", "awake", "private"
{
    transform = Transform();
    icons = [
        spawn("DefaultHUD.Powerups.Icon").setIndex(0),
        spawn("DefaultHUD.Powerups.Icon").setIndex(1),
        spawn("DefaultHUD.Powerups.Icon").setIndex(2)
    ];

    state "main"
    {
        transform.position = Vector2(Screen.width - parent.transform.position.x, parent.transform.position.y);

        if(SurgeEngine.mobileMode) {
            offset = Vector2.left.scaledBy(Screen.width / 4);
            transform.position = transform.position.plus(offset);
        }

        state = "active";
    }

    state "active"
    {
        player = Player.active;

        // hide icons
        icons[0].hide();
        icons[1].hide();
        icons[2].hide();

        // show relevant icons
        c = 0;
        if(player.turbo)
            icons[c++].show("turbo");
        if(player.invincible)
            icons[c++].show("invincible");
        if(player.shield !== null)
            icons[c++].show(player.shield);
    }
}

object "DefaultHUD.Powerups.Icon" is "entity", "detached", "awake", "private"
{
    actor = Actor("Item Box Icon");
    transform = Transform();
    name2anim = {
        "invincible": 5,
        "turbo": 6,
        "shield": 8,
        "fire": 12,
        "thunder": 13,
        "water": 14,
        "acid": 15,
        "wind": 16
    };

    state "main"
    {
        actor.visible = false;
        actor.zindex = parent.parent.zindex;

        state = "active";
    }

    state "active"
    {
        if(Level.cleared)
            hide();
    }

    fun setIndex(index)
    {
        dx = actor.width * (1 + index);
        transform.localPosition = Vector2(-dx, 0).minus(actor.actionOffset);
        return this;
    }

    fun show(iconName)
    {
        if(name2anim.has(iconName)) {
            actor.anim = name2anim[iconName];
            actor.visible = !Level.cleared;
        }
    }

    fun hide()
    {
        actor.visible = false;
    }
}

object "DefaultHUD.PauseButton" is "entity", "detached", "awake", "private"
{
    transform = Transform();
    actor = Actor("Mobile Pause Button");
    unpressed = 0;
    pressed = 1;

    state "main"
    {
        transform.position = Vector2(Screen.width * 0.87, parent.transform.position.y - actor.actionOffset.y);
        actor.anim = unpressed;
        actor.zindex = parent.zindex;

        // display this pause button in mobile mode only
        if(!SurgeEngine.mobileMode) {
            actor.visible = false;
            state = "sleep";
        }
        else {
            actor.visible = true;
            state = "unpressed";
        }
    }

    state "unpressed"
    {
        // check if tapped
        if(isTappingButton()) {
            actor.anim = pressed;
            state = "pressed";
        }

        // check if blocked
        if(!canPause())
            state = "blocked";
    }

    state "pressed"
    {
        // check if the game should be paused or
        // if the button should just be released
        if(!Mouse.buttonDown("left")) {
            actor.anim = unpressed;
            actor.visible = false;
            state = "triggered";
        }
        else if(!isTappingButton()) {
            actor.anim = unpressed;
            state = "unpressed";
        }

        // check if blocked
        if(!canPause())
            state = "blocked";
    }

    state "triggered"
    {
        if(canPause())
            Level.pause();

        state = "paused";
    }

    state "paused"
    {
        actor.visible = true;
        state = "unpressed";
    }

    state "blocked"
    {
        if(canPause()) {
            actor.visible = true;
            state = "unpressed";
        }
        else {
            actor.visible = false;
        }
    }

    state "sleep"
    {
        // do nothing
    }

    fun canPause()
    {
        return !Level.cleared && Level.child("Debug Mode") === null;
    }

    fun isTappingButton()
    {
        return Mouse.buttonDown("left") && isOverlappingButton(Mouse.position);
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