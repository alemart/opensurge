// -----------------------------------------------------------------------------
// File: door.ss
// Description: script for a door
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Brick;
using SurgeEngine.Audio.Sound;

object "Door" is "entity", "gimmick"
{
    public color = "yellow"; // must be: "yellow", "green", "blue" or "red"

    openSfx = Sound("samples/door1.wav");
    closeSfx = Sound("samples/door2.wav");
    actor = Actor("Door");
    brick = Brick("Door Mask");

    state "main"
    {
        // pick the right animation
        selectedColor = String(color).toLowerCase();
        if(selectedColor == "yellow")
            actor.anim = 0;
        else if(selectedColor == "blue")
            actor.anim = 3;
        else if(selectedColor == "red")
            actor.anim = 6;
        else if(selectedColor == "green")
            actor.anim = 9;
        else
            actor.anim = 0;

        // done
        state = "closed";
    }

    state "closed"
    {
        brick.enabled = true;
    }

    state "open"
    {
        brick.enabled = false;
    }

    fun open()
    {
        if(state != "open") {
            actor.anim = 3 * Math.floor(actor.anim / 3) + 1;
            openSfx.play();
            state = "open";
        }
    }

    fun close()
    {
        if(state == "open") {
            actor.anim = 3 * Math.floor(actor.anim / 3) + 2;
            closeSfx.play();
            state = "closed";
        }
    }

    fun toggle()
    {
        if(isOpen())
            close();
        else
            open();
    }

    fun isOpen()
    {
        return (state == "open");
    }
}