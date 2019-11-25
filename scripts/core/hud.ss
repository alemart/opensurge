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
    transform = Transform();

    state "main"
    {
        if(Level.cleared)
            state = "cleared";
    }

    state "cleared"
    {
        transform.move(-Screen.width * Time.delta, 0);
    }

    fun constructor()
    {
        transform.position = Vector2(16, 8);
        score.transform.localPosition = Vector2.zero;
        timer.transform.localPosition = Vector2(0, 16);
        collectibles.transform.localPosition = Vector2(0, 32);
        lives.transform.localPosition = Vector2(0, Screen.height - 29);
    }
}

object "DefaultHUD.Score" is "entity", "detached", "awake", "private"
{
    public transform = Transform();
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
    public transform = Transform();
    label = Text("HUD");
    value = Text("HUD");
    timer = 0.0;

    state "main"
    {
        // update timer
        timer += Time.delta;

        // update HUD
        seconds = Math.floor(timer);
        minutes = Math.floor(seconds / 60);
        sec = Math.mod(seconds, 60);
        dsec = Math.floor((timer - seconds) * 100);
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
    public transform = Transform();
    label = Text("HUD");
    value = Text("HUD");
    blinkTime = 0.35;

    state "main"
    {
        label.text = "<color=ffee11>$HUD_COLLECTIBLES</color>";
        value.text = Player.active.collectibles;
        if(Player.active.collectibles == 0) {
            if(timeout(blinkTime))
                state = "blink";
        }
    }

    state "blink"
    {
        label.text = "<color=ff0055>$HUD_COLLECTIBLES</color>";
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
    public transform = Transform();
    icon = Actor("Life Counter");
    value = Text("HUD");

    state "main"
    {
        value.text = Player.active.lives;
        icon.anim = animId(Player.active.name);
    }

    // given a player name, get the corresponding
    // animation ID of the "Life Counter" sprite
    fun animId(playerName)
    {
        if(playerName == "Surge")
            return 0;
        else if(playerName == "Neon")
            return 1;
        else if(playerName == "Charge")
            return 2;
        else if(playerName == "Tux")
            return 3;
        else
            return 14; // unknown player
    }

    fun constructor()
    {
        value.offset = Vector2(24, 0);
        icon.offset = Vector2(9, 4);
        icon.zindex = value.zindex = 1000.0;
    }
}