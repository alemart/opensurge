// -----------------------------------------------------------------------------
// File: boss_music.ss
// Description: a script that handles the Boss Music
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Audio.Music;
using SurgeEngine.Level;

object "Boss Music"
{
    music = Music("musics/boss.ogg");
    fadeTime = 0.5; // seconds
    timer = 0.0;

    state "main"
    {
    }

    state "playing"
    {
        // loop music
        if(Level.music.playing)
            music.play();
    }

    state "fading to boss"
    {
        if(Level.music.playing) {
            timer += Time.delta / fadeTime;
            Level.music.volume = 1.0 - timer;
            if(timer >= 1.0) {
                Level.music.stop();
                music.play();
                state = "playing";
            }
        }
        else
            state = "playing";
    }

    state "fading from boss"
    {
        if(music.playing) {
            timer += Time.delta / fadeTime;
            music.volume = 1.0 - timer;
            if(timer >= 1.0) {
                music.stop();
                state = "main";
            }
        }
        else
            state = "main";
    }

    fun play()
    {
        timer = 0.0;
        state = "fading to boss";
    }

    fun stop()
    {
        timer = 0.0;
        state = "fading from boss";
    }

    fun get_playing()
    {
        return music.playing;
    }
}

