// -----------------------------------------------------------------------------
// File: bubbles.ss
// Description: water bubbles (breathing)
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
object "Bubbles" is "entity", "basic"
{
    transform = Transform();
    bubbles = Actor("Bubbles");
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
        if(++cnt < 16) {
            x = transform.position.x;
            y = transform.position.y - 4;
            size = (Math.random() <= 0.5) ? "sm" : "xs";
            Level.spawn("WaterBubble").sized(size).at(x, y);
        }
        else {
            x = transform.position.x;
            y = transform.position.y - 4;
            Level.spawn("WaterBubble").sized("lg").at(x, y).withComponent("BreathableBubble");
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
object "BreathableBubble" is "collider"
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
    bubbles = spawn("Bubbles");

    state "main"
    {
    }
}