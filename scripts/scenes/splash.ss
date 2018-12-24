// -----------------------------------------------------------------------------
// File: splash.ss
// Description: a startup object for displaying a Splash Screen
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Level;
using SurgeEngine.Actor;
using SurgeEngine.Input;
using SurgeEngine.Video.Screen;

object "Splash"
{
    input = Input(null);
    fader = spawn("SplashFader");
    waitTime = 2.0; fadeTime = 0.5;

    state "main"     { fader.fadeIn(fadeTime); state = "fade in"; }
    state "fade in"  { if(timeout(fadeTime)) state = "wait"; }
    state "wait"     { if(timeout(waitTime) || skip()) { fader.fadeOut(fadeTime); state = "fade out"; } }
    state "fade out" { if(timeout(fadeTime)) state = "done"; }
    state "done"     { Level.finish(); }

    fun skip() { return input.buttonPressed("fire1") || input.buttonPressed("fire3") || input.buttonPressed("fire4"); }
}

// TODO: fader object
object "SplashFader" is "entity", "detached", "awake", "private"
{
    transform = Transform();
    actor = Actor("SD_HUDBLACKSCREEN");
    fadeTime = 1.0;

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

    fun constructor()
    {
        transform.position = Vector2.zero;
        actor.zindex = 1001.0;
        actor.visible = false;
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
}