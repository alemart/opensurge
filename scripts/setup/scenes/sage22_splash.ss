// -----------------------------------------------------------------------------
// File: sage22_splash.ss
// Description: script for the SAGE 22 Splash Screen
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

//
// The art is from the SAGE 22 Media Kit, licensed under CC-BY 4.0 by its authors.
//
// Credits of the SAGE 22 Media Kit:
//
// SAGE 2022 LOGO: P3DRO, RummySM
// SAGE 2022 SPRITE LOGO: VAdaPEGA
// SAGE 2022 SPLASH SCREEN: RummySM
// SAGE 2022 JINGLE: dconn
// SAGE 2022 BANNER: RummySM, VAdaPEGA​
//

using SurgeEngine.Audio.Music;
using SurgeEngine.Input;
using SurgeEngine.Level;

object "SAGE 22 Splash Screen" is "setup"
{
    jingle = Music("musics/sage22_splash.ogg");
    fader = spawn("Fader").setFadeTime(0.5);
    input = Input("default");
    secondsToWait = 9.0;

    state "main"
    {
        jingle.play();
        state = "waiting";
    }

    state "waiting"
    {
        if(timeout(secondsToWait) || shouldSkip()) {
            fader.fadeOut();
            state = "fading out";
        }
    }

    state "fading out"
    {
        jingle.volume -= Time.delta / fader.fadeTime;
        if(timeout(fader.fadeTime)) {
            jingle.stop();
            state = "finished";
        }
    }

    state "finished"
    {
        Level.loadNext();
    }

    fun shouldSkip()
    {
        return (
            input.buttonPressed("fire1") ||
            input.buttonPressed("fire3") ||
            input.buttonPressed("fire4")
        );
    }
}