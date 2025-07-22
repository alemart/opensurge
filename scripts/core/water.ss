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
using SurgeEngine.Collisions.CollisionBall;

// how to spawn a Water Bubble:
// bubble = Level.spawnEntity("Water Bubble", position).setSize("xs" | "sm" | "md" | "lg");
// [...]
// bubble.burst(); // destroy
object "Water Bubble" is "entity", "private", "disposable"
{
    transform = Transform();
    amplitude = 2 + Math.random() * 2;
    bubble = Actor("Water Bubble");
    collider = null;
    twoPi = Math.pi * 2;
    phi = twoPi * Math.random();
    t = 0;
    hy = 16;

    state "main"
    {
        state = "move";
    }

    state "move"
    {
        // timer
        dt = Time.delta;
        t += dt;

        // move upwards
        transform.translateBy((amplitude * twoPi) * Math.cos(twoPi * t + phi) * dt, -30 * dt); // chain rule

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
        hy = bubble.animation.hotSpot.y / 2;
    }

    // burst the bubble
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
            hy = bubble.animation.hotSpot.y / 2;
        }
        else if(size == "sm") {
            bubble.anim = 0;
            hy = bubble.animation.hotSpot.y / 2;
        }
        else if(size == "md") {
            bubble.anim = 3;
            hy = bubble.animation.hotSpot.y / 2;
        }
        else if(size == "lg") {
            bubble.anim = 1;
            hy = bubble.animation.hotSpot.y;
        }
        return this;
    }

    // make the bubble breathable
    fun makeBreathable()
    {
        if(collider !== null)
            return this;

        collider = CollisionBall(10);
        return this;
    }

    // dynamically add components
    // (this function is obsolete since v0.6.1. It is kept for backwards compatibility)
    fun addComponent(componentName)
    {
        makeBreathable(); // no components other than "BreathableBubble" were available
        return this;
    }

    // breathe the bubble
    fun onOverlap(otherCollider)
    {
        // wait for the bubble to grow
        if(t < 2.0)
            return;

        // breathe
        if(otherCollider.entity.hasTag("player")) {
            player = otherCollider.entity;
            if(player.shield != "water") {
                player.breathe();
                this.burst();

                // since the bubble takes a little while to burst,
                // two players can breathe the same bubble! Looks nice!
                //collider.enabled = false;
            }
        }
    }
}

object "Water Splash" is "entity", "private", "disposable"
{
    actor = Actor("Water Splash");
    inSfx = Sound("samples/water_in.wav");
    outSfx = Sound("samples/water_out.wav");

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

    fun setPlayer(player)
    {
        assert(player !== null);

        if(player.underwater)
            inSfx.play();
        else
            outSfx.play();

        return this;
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
object "Water Controller" is "private", "awake", "detached", "entity"
{
    underwaterTimer = spawn("WaterController.UnderwaterTimer");
    surfaceAnimation = spawn("WaterController.SurfaceAnimation");
    breathingBehaviors = [];
    splashListeners = [];

    state "main"
    {
        for(i = Player.count - 1; i >= 0; i--) {
            player = Player[i];

            splashListener = player.spawn("WaterController.SplashListener");
            splashListeners.push(splashListener);

            breathingBehavior = player.spawn("WaterController.BreathingBehavior");
            breathingBehaviors.push(breathingBehavior);
        }

        state = "done";
    }

    state "done"
    {
    }
}

// splash listener: creates a splash whenever the player gets underwater
object "WaterController.SplashListener"
{
    player = parent;

    state "main"
    {
        state = player.underwater ? "underwater" : "out of water";
    }

    state "underwater"
    {
        if(!player.underwater) {
            splash();
            state = "out of water";
        }
    }

    state "out of water"
    {
        if(player.underwater) {
            splash();
            state = "underwater";
        }
    }

    fun splash()
    {
        if(player.frozen)
            return;

        position = Vector2(player.transform.position.x, Level.waterlevel);
        Level.spawnEntity("Water Splash", position).setPlayer(player);
    }
}

// underwater timer: displayed when the player has been underwater for way too long
object "WaterController.UnderwaterTimer" is "entity", "private", "detached", "awake"
{
    transform = Transform();
    music = Music("musics/drowning.ogg");
    counter = Text("HUD Large");
    tick = Sound("samples/underwater_tick.wav");
    tickCount = 0;

    duration = 12; // display the timer for this amount of seconds before the player drowns
    warningTicks = 3; // how many warning ticks should we play when underwater?
    maxCounterValue = 5; // start counting from this number

    state "main"
    {
        player = Player.active;

        // the player is drowning
        if(player.drowning) {

            // we hide the counter but don't stop the drowning music
            counter.visible = false;
            return;

        }

        // the player is protected by the water shield
        if(player.shield == "water" || player.dying) {

            // if the player gets the water shield while the timer
            // is active, we hide the counter and stop the music
            if(music.playing || counter.visible) {
                counter.visible = false;
                music.stop();
            }

            // nothing to do
            return;
        }

        if(player.underwater) {
            // underwater warning tick sound
            c = Math.max(player.secondsToDrown - duration, 0);
            c /= Math.max(player.breathTime - duration, 0);
            c = Math.floor((1 - c) * (warningTicks + 0.5));
            if(c != tickCount) {
                if(c > 0)
                    tick.play();
                tickCount = c;
            }

            // breathing counter
            t = player.secondsToDrown;
            if(t > 0 && t < duration) {
                // show counter
                counter.text = "<color=ff0055>" + Math.floor(t * (1 + maxCounterValue) / duration) + "</color>";
                counter.visible = true;

                // music
                if(!music.playing)
                    music.play();
            }
            else {
                counter.visible = false;
                if(t <= 0)
                    Level.music.stop(); // drown
                else if(music.playing)
                    music.stop(); // got an air bubble
            }
        }
        else if(music.playing) {
            counter.visible = false;
            music.stop(); // got out of water
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
    player = parent;
    interval = 0;

    state "main"
    {
        if(player.underwater) {
            // spawn bubbles?
            if(timeout(interval) && player.shield != "water") {
                createBubble();
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
        if(player.underwater) {
            if(timeout(0.1)) {
                createBiggerBubble();
                state = "cooldownd";
            }
        }

        if(!player.drowning)
            state = "main"; // immortal player who has resurrected
    }

    state "cooldownd"
    {
        state = "drowning";
    }

    fun createBubble()
    {
        position = player.transform.position;
        size = Math.random() <= 0.5 ? "xs" : "sm";
        Level.spawnEntity(
            "Water Bubble",
            position.translatedBy(8 * player.direction, -16)
        ).setSize(size);
    }

    fun createBiggerBubble()
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
        Level.spawnEntity("Water Surface", Vector2.zero).setOffset(-1),
        Level.spawnEntity("Water Surface", Vector2.zero).setOffset(0),
        Level.spawnEntity("Water Surface", Vector2.zero).setOffset(1)
    ];

    state "main"
    {
    }
}
