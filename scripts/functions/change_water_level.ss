// -----------------------------------------------------------------------------
// File: water_level.ss
// Description: a function object that changes the water level
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;
using SurgeEngine.Camera;
using SurgeEngine.Video.Screen;
using SurgeEngine.Audio.Sound;

//
// Change Water Level is a function object that
// changes the water level with an animation
//
object "Change Water Level"
{
    animationTime = 3.0; // seconds
    newLevel = Level.waterlevel;
    oldLevel = Level.waterlevel;
    sfx = Sound("samples/waterlevel.wav");
    timer = 0;

    state "main"
    {
    }

    state "up"
    {
        // update timer
        timer += Time.delta / animationTime;
        if(timer >= 1)
            state = "complete";

        // update water level
        Level.waterlevel = Math.lerp(
            Math.min(oldLevel, Camera.position.y + Screen.height),
            Math.max(newLevel, Camera.position.y - Screen.height),
            timer
        );
    }

    state "down"
    {
        // update timer
        timer += Time.delta / animationTime;
        if(timer >= 1)
            state = "complete";

        // update water level
        Level.waterlevel = Math.lerp(
            Math.max(oldLevel, Camera.position.y - Screen.height),
            Math.min(newLevel, Camera.position.y + Screen.height),
            timer
        );
    }

    state "complete"
    {
        Level.waterlevel = newLevel;
        state = "main";
    }

    fun call(ypos)
    {
        if(state != "main")
            return;

        // is this change needed?
        ypos = Math.floor(ypos);
        if(Math.abs(Level.waterlevel - ypos) >= 1) {
            // initialize values
            newLevel = ypos;
            oldLevel = Level.waterlevel;
            timer = 0.0;

            // play sound
            sfx.play();

            // set animation direction
            if(newLevel < Level.waterlevel)
                state = "up"; // water rising
            else
                state = "down";
        }
    }
}