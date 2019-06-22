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
using SurgeEngine.Collisions.CollisionBall;

// these Bubbles help the player breathe (useful when underwater)
object "Water Bubbles" is "entity", "basic"
{
    transform = Transform();
    bubbles = Actor("Water Bubbles");
    hy = 16; cnt = 0;

    state "main"
    {
        if(timeout(0.5))
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

        if(++cnt < 16) {
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
            ).setSize("lg").addComponent("BreathableBubble");
            cnt = 0;
        }

        state = "main";
    }

    fun constructor()
    {
        bubbles.alpha = 0.5;
        bubbles.zindex = 0.99;
        bubbles.anim = 0;
        hy = bubbles.animation.hotspot.y / 2;
    }
}

// breathable behavior: bubble that the player can use to breathe
object "BreathableBubble"
{
    collider = CollisionBall(16);
    bubble = parent;

    state "main"
    {
        player = Player.active;
        if(player.shield != "water" && timeout(2)) {
            if(collider.collidesWith(player.collider)) {
                player.breathe();
                bubble.burst();
            }
        }
    }
}

// this is for retro compatibility
object "water.air_source" is "entity", "private"
{
    bubbles = spawn("Water Bubbles");

    state "main"
    {
    }
}