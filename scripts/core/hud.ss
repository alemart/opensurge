// -----------------------------------------------------------------------------
// File: hud.ss
// Description: the default Heads-Up Display for Open Surge
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Transform;
using SurgeEngine.Player;
using SurgeEngine.Camera;
using SurgeEngine.Vector2;
using SurgeEngine.Video;
using SurgeEngine.Actor;
using SurgeEngine.UI.Text;

object "Application" { x = spawn("DefaultHUD"); state "main" { } }
object "DefaultHUD" is "entity", "detached", "awake", "private"
{
    score = spawn("DefaultHUD.Score");
    timer = spawn("DefaultHUD.Time");
    collectibles = spawn("DefaultHUD.Collectibles");
    lives = spawn("DefaultHUD.Lives");
    transform = Transform();

    state "main"
    {
    }

    fun constructor()
    {
        transform.position = Vector2(16, 8);
        score.transform.localPosition = Vector2.zero;
        timer.transform.localPosition = Vector2(0, 16);
        collectibles.transform.localPosition = Vector2(0, 32);
        lives.transform.localPosition = Vector2(0, Video.screenHeight - 34);
    }
}

object "DefaultHUD.Score" is "entity", "detached", "awake", "private"
{
    public transform = Transform();
    label = Text("GoodNeighbors");
    value = Text("GoodNeighbors");

    state "main"
    {
        label.text = "<color=ffee55>$HUD_SCORE</color>";
        value.text = Player.active.score;
    }

    fun constructor()
    {
        label.zindex = value.zindex = 1000.0;
        label.transform.localPosition = Vector2(0, 0);
        value.transform.localPosition = Vector2(56, 0);
    }
}

object "DefaultHUD.Time" is "entity", "detached", "awake", "private"
{
    public transform = Transform();
    label = Text("GoodNeighbors");
    value = Text("GoodNeighbors");
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
        label.text = "<color=ffee55>$HUD_TIME</color>";
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
        label.transform.localPosition = Vector2(0, 0);
        value.transform.localPosition = Vector2(56, 0);
    }
}

object "DefaultHUD.Collectibles" is "entity", "detached", "awake", "private"
{
    public transform = Transform();
    label = Text("GoodNeighbors");
    value = Text("GoodNeighbors");
    blinkTime = 0.35;

    state "main"
    {
        label.text = "<color=ffee55>$HUD_COLLECTIBLES</color>";
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
        label.transform.localPosition = Vector2(0, 0);
        value.transform.localPosition = Vector2(56, 0);
    }
}

object "DefaultHUD.Lives" is "entity", "detached", "awake", "private"
{
    public transform = Transform();
    icon = Actor("SD_HUDHEAD");
    lives = Text("GoodNeighbors");

    state "main"
    {
        lives.text = Player.active.lives;
        icon.anim = iconId(Player.active.name);
    }

    fun iconId(playerName)
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
            return 0;
    }

    fun constructor()
    {
        icon.zindex = lives.zindex = 1000.0;
        lives.transform.localPosition = Vector2(24, 5);
    }
}