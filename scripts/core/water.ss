// -----------------------------------------------------------------------------
// File: water.ss
// Description: water scripts
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

// how to spawn a Water Bubble:
// bubble = Level.spawnEntity("Water Bubble", position).setSize("xs" | "sm" | "md" | "lg");
// [...]
// bubble.burst(); // destroy
object "Water Bubble" is "entity", "private", "disposable"
{
    transform = Transform();
    amplitude = 2 + Math.random() * 2;
    bubble = Actor("Water Bubble");
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

    fun burst()
    {
        bubble.anim = 2;
        state = "burst";
    }

    // set size
    fun setSize(size)
    {
        if(size == "xs") {
            bubble.anim = 4;
            hy = bubble.animation.hotspot.y / 2;
        }
        else if(size == "sm") {
            bubble.anim = 0;
            hy = bubble.animation.hotspot.y / 2;
        }
        else if(size == "md") {
            bubble.anim = 3;
            hy = bubble.animation.hotspot.y / 2;
        }
        else if(size == "lg") {
            bubble.anim = 1;
            hy = bubble.animation.hotspot.y;
        }
        return this;
    }

    // dynamically add components
    fun addComponent(componentName)
    {
        newComponent = spawn(componentName);
        if(components == null)
            components = [];
        components.push(newComponent);
        return this;
    }
}

object "Water Splash" is "entity", "private", "disposable"
{
    actor = Actor("Water Splash");

    state "main"
    {
        if(timeout(0.27))
            destroy();
    }

    fun constructor()
    {
        actor.zindex = 0.99;
        actor.alpha = 0.7;
    }
}

object "Water Surface" is "entity", "private", "awake"
{
    actor = Actor("Water Surface");
    transform = Transform();
    sw = actor.width;
    dx = 0;

    state "main"
    {
        xpos = (Math.floor(Camera.position.x / sw) + dx) * sw;
        transform.position = Vector2(xpos, Level.waterlevel);
    }

    fun constructor()
    {
        actor.zindex = 0.99;
        actor.alpha = 0.2;
    }

    fun setOffset(offset)
    {
        dx = offset;
        return this;
    }
}

// --------------------------------------------------------------

// this object controls water behavior: bubbles, breathing timer, and so on
object "Water Controller"
{
    breathingBehavior = spawn("WaterController.BreathingBehavior");
    underwaterTimer = spawn("WaterController.UnderwaterTimer");
    splashListener = spawn("WaterController.SplashListener");
    surfaceAnimation = spawn("WaterController.SurfaceAnimation");

    state "main"
    {
    }
}

// splash listener: creates a splash whenever the player gets underwater
object "WaterController.SplashListener"
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
        Level.spawnEntity("Water Splash", Vector2(player.transform.position.x, Level.waterlevel));
    }
}

// underwater timer: displayed when the player has been underwater for way too long
object "WaterController.UnderwaterTimer" is "entity", "private", "detached", "awake"
{
    transform = Transform();
    music = Music("musics/drowning.ogg");
    counter = Text("HUD Large");
    tick = Sound("samples/underwater_tick.wav");
    tickState = 0;

    seconds = 5; // when should we display the timer?
    tickCount = 3; // how many warning ticks should we play when underwater?

    state "main"
    {
        player = Player.active;
        if(player.shield == "water")
            return;
        if(player.underwater) {
            // underwater warning tick sound
            c = Math.max(player.secondsToDrown - seconds, 0);
            c /= Math.max(player.breathTime - seconds, 0);
            c = Math.floor((1 - c) * (tickCount + 0.5));
            if(c > tickState) {
                tickState = c;
                tick.play();
            }

            // breathing counter
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
                else if(music.playing)
                    music.stop(); // got an air bubble
            }
        }
        else {
            tickState = 0;
            if(music.playing) {
                counter.visible = false;
                music.stop();
            }
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
object "WaterController.BreathingBehavior"
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
            if(player.drowning)
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
        position = player.transform.position;
        size = Math.random() <= 0.5 ? "xs" : "sm";
        Level.spawnEntity(
            "Water Bubble",
            position.translatedBy(8 * player.direction, -16)
        ).setSize(size);
    }

    fun createDrownBubble(player)
    {
        position = player.transform.position;
        size = Math.random() <= 0.5 ? "md" : "sm";
        Level.spawnEntity(
            "Water Bubble",
            position.translatedBy(0, -16)
        ).setSize(size);
    }
}

// this object controls the surface animation of the water
object "WaterController.SurfaceAnimation"
{
    surfaces = [
        Level.spawn("Water Surface").setOffset(-1),
        Level.spawn("Water Surface").setOffset(0),
        Level.spawn("Water Surface").setOffset(1)
    ];

    state "main"
    {
    }
}