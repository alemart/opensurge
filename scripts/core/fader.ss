// -----------------------------------------------------------------------------
// File: fader.ss
// Description: simple fader object (fade-in & fade-out)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Actor;
using SurgeEngine.Video.Screen;

//
// How to use:
//
// object "FadeTest" {
//     fader = spawn("Fader");
//
//     state "main" {
//         if(timeout(2.0))
//             fader.fadeOut(1.0); // fade out - duration: 1 second
//     }
//
//     fun constructor() {
//         fader.fadeIn(0.5); // fade in - duration: 0.5s
//     }
// }
//
object "Fader" is "entity", "detached", "awake", "private"
{
    transform = Transform();
    actor = Actor("SD_HUDBLACKSCREEN");
    fadeTime = 1.0;
    zindex = Math.infinity; //1001;

    state "main"
    {
    }

    state "fade out"
    {
        actor.alpha += Time.delta / fadeTime;
        if(actor.alpha >= 1.0)
            state = "main";
    }

    state "fade in"
    {
        actor.alpha -= Time.delta / fadeTime;
        if(actor.alpha <= 0.0)
            state = "main";
    }

    fun fadeOut(t)
    {
        fadeTime = Math.max(t, 0.001);
        actor.alpha = 0.0;
        actor.visible = true;
        state = "fade out";
    }

    fun fadeIn(t)
    {
        fadeTime = Math.max(t, 0.001);
        actor.alpha = 1.0;
        actor.visible = true;
        state = "fade in";
    }

    fun constructor()
    {
        transform.position = Vector2.zero;
        actor.zindex = zindex;
        actor.visible = false;
    }
}