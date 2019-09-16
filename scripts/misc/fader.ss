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
// Fader is a transition effect used to fade the screen
//
// Functions:
// - fadeIn(): fade from black
// - fadeOut(): fade to black
//
// Properties:
// - fadeTime: number. Transition time, in seconds
//
// Example - how to use:
//
// object "FadeTest" {
//     fader = spawn("Fader");
//
//     state "main" {
//         fader.fadeIn();
//         state = "wait";
//     }
//
//     state "wait" {
//         if(timeout(fader.fadeTime + 2.0)) {
//             fader.fadeOut();
//             state = "done";
//         }
//     }
//
//     state "done" { }
// }
//
object "Fader" is "entity", "detached", "awake", "private"
{
    actor = Actor("Fade Effect");
    transform = Transform();
    fadeTime = 0.5; // seconds

    state "main"
    {
        // idle
    }

    state "fade out"
    {
        actor.alpha += Time.delta / fadeTime;
        if(actor.alpha >= 1.0) {
            actor.alpha = 1.0;
            state = "main";
        }
    }

    state "fade in"
    {
        actor.alpha -= Time.delta / fadeTime;
        if(actor.alpha <= 0.0) {
            actor.alpha = 0.0;
            actor.visible = false;
            state = "main";
        }
    }

    fun fadeOut()
    {
        actor.alpha = 0.0;
        actor.visible = true;
        state = "fade out";
    }

    fun fadeIn()
    {
        actor.alpha = 1.0;
        actor.visible = true;
        state = "fade in";
    }

    fun get_fadeTime()
    {
        return fadeTime;
    }

    fun set_fadeTime(seconds)
    {
        fadeTime = Math.max(seconds, 0.001);
    }

    fun constructor()
    {
        transform.position = Vector2.zero;
        actor.zindex = Math.infinity;
        actor.visible = false;
    }
}