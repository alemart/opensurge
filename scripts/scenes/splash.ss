// -----------------------------------------------------------------------------
// File: splash.ss
// Description: a startup object for displaying a Splash Screen
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Input;
using SurgeEngine.Level;

object "Splash"
{
    input = Input(null);
    fader = spawn("Fader");
    waitTime = 2.0;
    fadeTime = 0.5;

    state "main"     { fader.fadeIn(fadeTime); state = "fade in"; }
    state "fade in"  { if(timeout(fadeTime)) state = "wait"; }
    state "wait"     { if(timeout(waitTime) || skip()) { fader.fadeOut(fadeTime); state = "fade out"; } }
    state "fade out" { if(timeout(fadeTime)) state = "done"; }
    state "done"     { Level.finish(); }

    fun skip() { return input.buttonPressed("fire1") || input.buttonPressed("fire3") || input.buttonPressed("fire4"); }
}