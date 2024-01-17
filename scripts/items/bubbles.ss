// -----------------------------------------------------------------------------
// File: bubbles.ss
// Description: water bubbles (for breathing)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Actor;
using SurgeEngine.Level;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;

// these Bubbles help the player breathe (useful when underwater)
object "Water Bubbles" is "entity", "basic"
{
    transform = Transform();
    bubbles = Actor("Water Bubbles");
    bubbleTime = 0.5; // in seconds
    bigBubbleTime = 8.0; // in seconds
    maxCounter = Math.floor(bigBubbleTime / bubbleTime);
    counter = 0;
    hy = 16;

    state "main"
    {
        if(timeout(bubbleTime))
            state = "create bubble";
        else if(transform.position.y - hy <= Level.waterlevel)
            state = "hidden";
    }

    state "hidden"
    {
        bubbles.visible = false;
        if(transform.position.y - hy > Level.waterlevel) {
            bubbles.visible = true;
            state = "main";
        }
    }

    state "create bubble"
    {
        position = transform.position;

        if(++counter < maxCounter) {
            size = (Math.random() <= 0.5) ? "sm" : "xs";
            Level.spawnEntity(
                "Water Bubble",
                position.translatedBy(0, -4)
            ).setSize(size);
        }
        else {
            Level.spawnEntity(
                "Water Bubble",
                position.translatedBy(0, -4)
            ).setSize("lg").makeBreathable();
            counter = 0;
        }

        state = "main";
    }

    fun constructor()
    {
        bubbles.alpha = 0.5;
        bubbles.zindex = 0.99;
        bubbles.anim = 0;
        hy = bubbles.animation.hotSpot.y / 2;

        restartCounter();
    }

    fun onReset()
    {
        restartCounter();
    }

    fun restartCounter()
    {
        // bubbles should be spawned quickly
        // when this object becomes visible.
        anticipatedSeconds = 2.0;

        // players can trick this logic to
        // get an air bubble faster!
        counter = maxCounter - anticipatedSeconds / bubbleTime;
        counter = Math.max(0, Math.floor(counter));
    }
}

// this is for retro compatibility with Legacy Open Surge
object "water.air_source" is "entity", "private"
{
    bubbles = spawn("Water Bubbles");

    state "main"
    {
    }
}
