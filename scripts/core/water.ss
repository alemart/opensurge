// -----------------------------------------------------------------------------
// File: water.ss
// Description: water effects
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Actor;
using SurgeEngine.Level;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;
using SurgeEngine.UI.Text;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Audio.Music;
using SurgeEngine.Video.Screen;
using SurgeEngine.Camera;

// how to spawn a WaterBubble:
// bubble = Level.spawn("WaterBubble").at(x, y).sized("xs" | "sm" | "md" | "lg");
// [...]
// bubble.burst(); // destroy
object "WaterBubble" is "entity", "private", "disposable"
{
    transform = Transform();
    amplitude = 2 + Math.random() * 2;
    bubble = Actor("SD_WATERAIRBUBBLE");
    hy = 16; t = 0;
    components = null;

    state "main"
    {
        state = "move";
    }

    state "move"
    {
        // move upwards
        dt = Time.delta; t += dt;
        transform.move((amplitude * 6.2832) * Math.cos(6.2832 * t) * dt, -30 * dt); // chain rule

        // got out of water?
        if(transform.position.y - hy < Level.waterlevel)
            destroy();
    }

    state "burst"
    {
        if(bubble.animation.finished)
            destroy();
    }

    fun constructor()
    {
        bubble.alpha = 0.5;
        bubble.zindex = 0.99;
        bubble.anim = 0;
        hy = bubble.animation.hotspot.y / 2;
    }

    fun at(x, y)
    {
        transform.position = Vector2(x, y);
        return this;
    }

    fun sized(size)
    {
        if(size == "xs")
            bubble.anim = 4;
        else if(size == "sm")
            bubble.anim = 0;
        else if(size == "md")
            bubble.anim = 3;
        else if(size == "lg")
            bubble.anim = 1;
        hy = bubble.animation.hotspot.y / 2;
        return this;
    }

    fun withComponent(componentName)
    {
        newComponent = spawn(componentName);
        if(components == null)
            components = [];
        components.push(newComponent);
        return this;
    }

    fun burst()
    {
        bubble.anim = 2;
        state = "burst";
    }
}

// how to spawn a WaterSplash:
// splash = Level.spawn("WaterSplash").at(x, y);
object "WaterSplash" is "entity", "private", "disposable"
{
    transform = Transform();
    splash = Actor("SD_WATERSPLASH");

    state "main"
    {
        if(timeout(0.27))
            destroy();
    }

    fun at(x, y)
    {
        transform.position = Vector2(x, y);
        return this;
    }

    fun constructor()
    {
        splash.zindex = 0.99;
        splash.alpha = 0.7;
    }
}


// this object controls water behavior: bubbles, breathing timer, and so on
object "DefaultWaterController"
{
    breathingBehavior = spawn("DefaultWaterController.BreathingBehavior");
    underwaterTimer = spawn("DefaultWaterController.UnderwaterTimer");
    splashListener = spawn("DefaultWaterController.SplashListener");

    state "main"
    {
    }
}

// splash listener: creates a splash whenever the player gets underwater
object "DefaultWaterController.SplashListener"
{
    state "main"
    {
        state = Player.active.underwater ? "underwater" : "out of water";
    }

    state "underwater"
    {
        if(!Player.active.underwater) {
            splash(Player.active);
            state = "out of water";
        }
    }

    state "out of water"
    {
        if(Player.active.underwater) {
            splash(Player.active);
            state = "underwater";
        }
    }

    fun splash(player)
    {
        Level.spawn("WaterSplash").at(player.transform.position.x, Level.waterlevel);
    }
}

// underwater timer: displayed when the player has been underwater for way too long
object "DefaultWaterController.UnderwaterTimer" is "entity", "private", "detached", "awake"
{
    transform = Transform();
    music = Music("musics/drowning.ogg");
    counter = Text("GoodNeighborsLarge");
    seconds = 5; // when should we display the timer?

    state "main"
    {
        player = Player.active;
        if(player.shield == "water")
            return;
        if(player.underwater) {
            t = player.secondsToDrown;
            if(t > 0 && t <= seconds) {
                // show counter
                counter.text = "<color=ff0055>" + Math.ceil(t) + "</color>";
                counter.visible = true;

                // music
                if(!music.playing)
                    music.play();
            }
            else {
                counter.visible = false;
                if(t <= 0)
                    Level.music.stop();
            }
        }
        else if(music.playing) {
            counter.visible = false;
            music.stop();
        }
    }

    fun constructor()
    {
        transform.position = Vector2(Screen.width / 2, Screen.height / 2 - 16);
        counter.align = "center";
        counter.visible = false;
        counter.zindex = 1000.0;
    }
}

// this object creates bubbles whenever the player is underwater
object "DefaultWaterController.BreathingBehavior"
{
    interval = 0;

    state "main"
    {
        player = Player.active;
        if(player.underwater) {
            // spawn bubbles?
            if(timeout(interval) && player.shield != "water") {
                createBubble(player);
                state = "cooldown";
            }

            // drowning sensor
            if(player.activity == "drowning")
                state = "drowning";
        }
    }

    state "cooldown"
    {
        interval = 1.0 + Math.random();
        state = "main";
    }

    state "drowning"
    {
        player = Player.active;
        if(player.underwater) {
            if(timeout(0.1)) {
                createDrownBubble(player);
                state = "cooldownd";
            }
        }
    }

    state "cooldownd"
    {
        state = "drowning";
    }

    fun createBubble(player)
    {
        pos = player.transform.position;
        size = Math.random() <= 0.5 ? "xs" : "sm";
        Level.spawn("WaterBubble").at(pos.x + 8 * player.direction, pos.y - 16).sized(size);
    }

    fun createDrownBubble(player)
    {
        pos = player.transform.position;
        size = Math.random() <= 0.5 ? "md" : "sm";
        Level.spawn("WaterBubble").at(pos.x, pos.y - 16).sized(size);
    }
}