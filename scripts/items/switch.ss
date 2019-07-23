// -----------------------------------------------------------------------------
// File: switch.ss
// Description: script for an on/off switch
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Brick;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Collisions.CollisionBox;
using SurgeEngine.Events.Event;

object "Switch" is "entity", "gimmick"
{
    public sticky = true; // a sticky switch is activated only once
    public onActivate = Event();
    public onDeactivate = Event();
    public color = "yellow"; // must be: "yellow", "green", "blue" or "red"

    sfx = Sound("samples/switch.wav");
    actor = Actor("Switch");
    brick = Brick("Switch Mask");
    collider = CollisionBox(22, actor.height).setAnchor(0.5, 1);
    pressed = false;

    state "main"
    {
        // pick the right animation
        selectedColor = String(color).toLowerCase();
        if(selectedColor == "yellow")
            actor.anim = 0;
        else if(selectedColor == "blue")
            actor.anim = 2;
        else if(selectedColor == "red")
            actor.anim = 4;
        else if(selectedColor == "green")
            actor.anim = 6;
        else
            actor.anim = 0;

        // done
        state = "idle";
    }

    state "idle"
    {
    }

    state "active"
    {
        if(!sticky && !pressed)
            deactivate();
        pressed = false;
    }

    fun onOverlap(otherCollider)
    {
        entity = otherCollider.entity;
        if(entity.hasTag("player") || entity.hasTag("blocky")) {
            pressed = true;
            activate();
        }
    }

    fun activate()
    {
        if(state != "active") {
            actor.anim++;
            sfx.play();
            onActivate.call();
            state = "active";
        }
    }

    fun deactivate()
    {
        if(state == "active") {
            actor.anim--;
            onDeactivate.call();
            state = "idle";
        }
    }

    /*
    fun get_active()
    {
        return (state == "active");
    }
    */
}