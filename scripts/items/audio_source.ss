// -----------------------------------------------------------------------------
// File: audio_source.ss
// Description: spatial audio source
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;
using SurgeEngine.Player;
using SurgeEngine.Transform;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Video.Screen;

//
// Audio Source
//
// This is a spatial audio source. The sound is attenuated according to the
// distance between the active player and the audio source. In this version,
// a linear falloff is used.
//
// Properties:
// - sound: string. Path to a .wav file in the samples/ folder.
// - type: string. Either "line" or "point" ("line" plays nicely on a platformer).
// - mindist: number. Within a distance of mindist pixels, the sound will stay the loudest.
// - maxdist: number. Outside the region of maxdist pixels, the sound will be silent.
// - volume: number. Base volume: a number between 0 and 1. Usually set to 1.
// - enabled: boolean. Whether the audio source is enabled or not.
//
object "Audio Source" is "entity", "special"
{
    public sound = null;
    public type = "line";
    public mindist = 256;
    public maxdist = 512;
    mixer = Level.child("Audio Mixer") || Level.spawn("Audio Mixer");
    transform = Transform();
    distance = null;
    snd = null;
    vol = 0;
    basevol = 1;

    // get the sound effect
    state "main"
    {
        if(sound != null) {
            if(typeof(sound) == "string") {
                distance = distance || ((type == "line") ?
                    spawn("Audio Source - Horizontal Distance") :
                    spawn("Audio Source - Euclidean Distance")
                );
                snd = sound;
                state = "playing";
            }
        }
    }

    // play the sound effect
    state "playing"
    {
        vol = volumeAt(Player.active.transform.position);
        mixer.notify(snd, vol);
    }

    // this audio source is disabled
    state "disabled"
    {
        if(snd !== null)
            mixer.notify(snd, 0.0);
    }

    // stop the sound
    fun onReset()
    {
        if(snd !== null)
            mixer.notify(snd, 0.0);
    }

    // compute the volume of the audio source at a certain position
    fun volumeAt(position)
    {
        dist = distance(transform.position, position);
        dist = Math.clamp(dist, mindist, maxdist);
        return basevol * (maxdist - dist) / (maxdist - mindist);
    }

    // get the base volume
    fun get_volume()
    {
        return basevol;
    }

    // set the base volume
    fun set_volume(value)
    {
        basevol = Math.clamp(value, 0, 1);
    }

    // is the audio source enabled?
    fun get_enabled()
    {
        return state != "disabled";
    }

    // enable/disable the audio source
    fun set_enabled(enabled)
    {
        if(enabled) {
            if(state == "disabled")
                state = "main";
        }
        else
            state = "disabled";
    }
}

// Mixes multiple audio sources
object "Audio Mixer"
{
    channel = {};

    fun notify(snd, vol)
    {
        if((c = channel[snd]) === null)
            channel[snd] = spawn("Audio Mixer - Channel").setSound(snd).increaseVolume(vol);
        else
            c.increaseVolume(vol);
    }
}

object "Audio Mixer - Channel"
{
    sound = null;
    volume = 0.0;

    state "main"
    {
        assert(sound !== null);

        if(volume == 0.0) {
            sound.volume = 0.0;
            //sound.stop(); // will restart the sound if the player moves back and forth
            return;
        }

        sound.volume = volume;
        if(!sound.playing)
            sound.play();

        volume = 0.0; // need to increaseVolume() every frame
    }

    fun setSound(filepath)
    {
        sound = Sound(filepath);
        return this;
    }

    fun increaseVolume(vol)
    {
        vol = Math.clamp(vol, 0.0, 1.0);
        volume = Math.max(volume, vol);
        return this;
    }

    fun destructor()
    {
        assert(sound !== null);
        sound.stop();
    }
}

// Computes the horizontal distance between
// of a and b, both Vector2 objects
object "Audio Source - Horizontal Distance"
{
    margin = Screen.height / 2;

    fun call(a, b)
    {
        dy = Math.abs(a.y - b.y) - margin;

        if(dy < 0)
            return Math.abs(a.x - b.x);
        else
            return Math.abs(a.x - b.x) + dy * 2;
    }

    fun setLineHeight(value)
    {
        margin = Math.max(value, 0) / 2;
        return this;
    }
}

// Computes the Euclidean distance between
// a and b, both Vector2 objects
object "Audio Source - Euclidean Distance"
{
    fun call(a, b)
    {
        return a.distanceTo(b);
    }
}