// -----------------------------------------------------------------------------
// File: title_card.ss
// Description: default Title Card animation
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

//
// The Default Title Card animation is divided into 5 phases:
//
// phase 0: warming up
// phase 1: appearing
// phase 2: sustaining
// phase 3: disappearing
// phase 4: finishing up
//
// These phases are played sequentially. The level name and zone number (act)
// appear during phases 1, 2 and 3. After phase 4, the title card disappears.
//
// Creating a new title card can be done simply: provide a new spritesheet and
// override the sprite script. Be mindful of the above phases.
//

using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Level;
using SurgeEngine.Actor;
using SurgeEngine.Player;
using SurgeEngine.UI.Text;
using SurgeEngine.Video.Screen;
using SurgeEngine.Input.MobileGamepad;

object "Default Title Card" is "entity", "awake", "detached", "private"
{
    public readonly zindex = 1001.0;
    transform = Transform();
    actor = Actor("Default Title Card");
    levelInfo = spawn("Default Title Card - Level Info").setTargetPosition(actor.animation.actionOffset);
    timeBlocker = spawn("Default Title Card - Time Blocker");
    playerBlocker = spawn("Default Title Card - Player Blocker");

    state "main"
    {
        MobileGamepad.fadeOut();
        actor.anim = 0;
        state = "warming up";
    }

    state "warming up"
    {
        if(actor.animation.finished) {
            actor.anim = 1;
            levelInfo.appear(animationTime());
            state = "appearing";
        }
    }

    state "appearing"
    {
        if(actor.animation.finished) {
            actor.anim = 2;
            state = "sustaining";
        }
    }

    state "sustaining"
    {
        if(actor.animation.finished) {
            actor.anim = 3;
            levelInfo.disappear(animationTime());
            state = "disappearing";
        }
    }

    state "disappearing"
    {
        MobileGamepad.fadeIn();
        if(actor.animation.finished) {
            actor.anim = 4;
            state = "finishing up";
        }
    }

    state "finishing up"
    {
        if(actor.animation.finished) {

            // we're done with this Title Card!
            destroy();

        }
    }

    // how long does it take for the current animation to complete?
    fun animationTime()
    {
        // note: actor.animation.speedFactor is 1
        return actor.animation.frameCount / actor.animation.fps;
    }

    fun constructor()
    {
        transform.position = Vector2.zero;
        actor.zindex = zindex;
    }
}

// Level Info: Level Name & Zone Number
object "Default Title Card - Level Info" is "entity", "awake", "detached", "private"
{
    public readonly zindex = parent.zindex;
    transform = Transform();
    levelName = spawn("Default Title Card - Level Name");
    zoneNumber = spawn("Default Title Card - Zone Number");
    initialPosition = Vector2.zero;
    targetPosition = Vector2.zero;
    time = 0.0;
    duration = 1.0;

    state "main"
    {
        // initialize
        transform.localPosition = initialPosition = Vector2(
            Screen.width + levelName.size.x / 2,
            targetPosition.y
        );

        state = "waiting";
    }

    state "waiting"
    {
        // do nothing
    }

    state "appearing"
    {
        time += Time.delta;
        t = time / duration;

        x = Math.smoothstep(initialPosition.x, targetPosition.x, t);
        y = Math.smoothstep(initialPosition.y, targetPosition.y, t);
        transform.localPosition = Vector2(x, y);

        if(time >= duration)
            state = "waiting";
    }

    state "disappearing"
    {
        time += Time.delta;
        t = time / duration;

        x = Math.smoothstep(targetPosition.x, initialPosition.x, t);
        y = Math.smoothstep(targetPosition.y, initialPosition.y, t);
        transform.localPosition = Vector2(x, y);

        if(time >= duration)
            state = "waiting";
    }

    fun appear(seconds)
    {
        duration = Math.max(0.01, seconds);
        time = 0.0;
        state = "appearing";
    }

    fun disappear(seconds)
    {
        duration = Math.max(0.01, seconds);
        time = 0.0;
        state = "disappearing";
    }

    fun setTargetPosition(target)
    {
        targetPosition = target;
        return this;
    }
}

object "Default Title Card - Level Name" is "entity", "awake", "detached", "private"
{
    label = Text("Default Title Card - Level Name");

    fun get_size()
    {
        return label.size;
    }

    fun constructor()
    {
        label.text = Level.name;
        label.align = "center";
        label.zindex = parent.zindex + 0.1;
    }
}

object "Default Title Card - Zone Number" is "entity", "awake", "detached", "private"
{
    actor = Actor("Default Title Card - Zone Number");

    fun constructor()
    {
        actor.anim = Level.act;
        actor.zindex = parent.zindex + 0.2;
        if(!actor.animation.exists)
            actor.visible = false;
    }
}

// Do not advance time until we are finished with the Title Card animation
object "Default Title Card - Time Blocker"
{
    state "main"
    {
        Level.time = 0.0;
    }
}

// Block player control until we are finished with the Title Card animation
object "Default Title Card - Player Blocker"
{
    player = Player.active;

    fun constructor()
    {
        player.input.enabled = false;
    }

    fun destructor()
    {
        player.input.enabled = true;
    }
}