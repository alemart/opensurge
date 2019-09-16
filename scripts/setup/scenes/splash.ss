// -----------------------------------------------------------------------------
// File: splash.ss
// Description: a setup object for displaying a Splash Screen
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;
using SurgeEngine.Player;

object "Splash"
{
    input = Player.active.input;
    fader = spawn("Fader");
    waitTime = 2.0;

    state "main"     { fader.fadeIn(); state = "fade in"; }
    state "fade in"  { if(timeout(fader.fadeTime)) state = "wait"; }
    state "wait"     { if(timeout(waitTime) || skip()) { fader.fadeOut(); state = "fade out"; } }
    state "fade out" { if(timeout(fader.fadeTime)) state = "done"; }
    state "done"     { Level.loadNext(); }

    fun skip() { return input.buttonPressed("fire1") || input.buttonPressed("fire3") || input.buttonPressed("fire4"); }
}