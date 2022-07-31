// -----------------------------------------------------------------------------
// File: hud.ss
// Description: the default Heads-Up Display for Open Surge
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Transform;
using SurgeEngine.Player;
using SurgeEngine.Vector2;
using SurgeEngine.Actor;
using SurgeEngine.Level;
using SurgeEngine.UI.Text;
using SurgeEngine.Video.Screen;

object "Default HUD" is "entity", "detached", "awake", "private"
{
    score = spawn("DefaultHUD.Score");
    timer = spawn("DefaultHUD.Time");
    collectibles = spawn("DefaultHUD.Collectibles");
    lives = spawn("DefaultHUD.Lives");
    powerups = spawn("DefaultHUD.Powerups");
    transform = Transform();

    state "main"
    {
        if(Level.cleared)
            state = "cleared";
    }

    state "cleared"
    {
        transform.translateBy(-Screen.width * Time.delta, 0);
    }

    fun constructor()
    {
        transform.position = Vector2(16, 8);
        score.transform.localPosition = Vector2.zero;
        timer.transform.localPosition = Vector2(0, 16);
        collectibles.transform.localPosition = Vector2(0, 32);
        lives.transform.localPosition = Vector2(0, Screen.height - 29);
        powerups.transform.localPosition = Vector2(Screen.width - 32, 0);
    }
}

object "DefaultHUD.Score" is "entity", "detached", "awake", "private"
{
    public readonly transform = Transform();
    label = Text("HUD");
    value = Text("HUD");

    state "main"
    {
        label.text = "<color=ffee11>$HUD_SCORE</color>";
        value.text = Player.active.score;
    }

    fun constructor()
    {
        label.zindex = value.zindex = 1000.0;
        label.offset = Vector2(0, 0);
        value.offset = Vector2(56, 0);
    }
}

object "DefaultHUD.Time" is "entity", "detached", "awake", "private"
{
    public readonly transform = Transform();
    label = Text("HUD");
    value = Text("HUD");

    state "main"
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

    fun pad(x)
    {
        if(x < 10)
            return "0" + x;
        else
            return x;
    }

    fun constructor()
    {
        label.zindex = value.zindex = 1000.0;
        label.offset = Vector2(0, 0);
        value.offset = Vector2(56, 0);
    }
}

object "DefaultHUD.Collectibles" is "entity", "detached", "awake", "private"
{
    public readonly transform = Transform();
    label = Text("HUD");
    value = Text("HUD");
    blinkTime = 0.35;

    state "main"
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

    fun constructor()
    {
        label.zindex = value.zindex = 1000.0;
        label.offset = Vector2(0, 0);
        value.offset = Vector2(56, 0);
    }
}

object "DefaultHUD.Lives" is "entity", "detached", "awake", "private"
{
    public readonly transform = Transform();
    value = Text("HUD");
    icon = null;
    currentPlayer = null;
    zindex = 1000;

    state "main"
    {
        value.text = Player.active.lives;
        if(Player.active.name !== currentPlayer) {
            currentPlayer = Player.active.name;
            updateIcon(currentPlayer);
        }
    }

    fun constructor()
    {
        value.zindex = zindex;
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
        icon.zindex = zindex;
        value.offset = Vector2(icon.width, 0);
    }
}

object "DefaultHUD.Powerups" is "entity", "detached", "awake", "private"
{
    public readonly transform = Transform();
    icons = [
        spawn("DefaultHUD.Powerups.Icon").setIndex(0),
        spawn("DefaultHUD.Powerups.Icon").setIndex(1),
        spawn("DefaultHUD.Powerups.Icon").setIndex(2)
    ];

    state "main"
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
        if(Level.cleared)
            hide();
    }

    fun constructor()
    {
        actor.visible = false;
        actor.zindex = 1000.0;
    }

    fun setIndex(index)
    {
        hotSpot = actor.animation.hotSpot;
        offset = Vector2(-hotSpot.x, hotSpot.y);
        transform.localPosition = Vector2(1.25 * actor.width * -index, 0).plus(offset);
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