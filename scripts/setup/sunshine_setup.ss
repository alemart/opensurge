// -----------------------------------------------------------------------------
// File: sunshine_setup.ss
// Description: setup object for Sunshine Paradise
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Lang;
using SurgeEngine.Level;
using SurgeEngine.Player;
using SurgeEngine.Vector2;
using SurgeEngine.Audio.Music;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Events.EventList;
using SurgeEngine.Events.EventChain;
using SurgeEngine.Events.EntityEvent;
using SurgeEngine.Events.DelayedEvent;
using SurgeEngine.Events.FunctionEvent;

object "Sunshine Setup"
{
    config = {

        //
        // all zones / acts
        //
        "*": {
            "Bridge": {
                "anim": 0
            },
            "Audio Source": {
                "sound": "samples/waterfall.wav"
            },
            "Fish": {
                "anim": 0
            },
            "Mosquito": {
                "anim": 0
            },
            "Zipline Grabber": {
                "anim": 1
            }
        },

        //
        // zone 1 only
        //
        "1": {
            // Bridge
            "31689decb2eab796": {
                "length": 8
            },
        },

        //
        // zone 2 only
        //
        "2": {

        },

        //
        // zone 3 only
        //
        "3": {

        }

    };

    // -------------------------------------------------------------------------

    state "main"
    {
    }

    // setup the entities
    fun constructor()
    {
        zone = String(Level.act);
        Level.setup(config["*"] || { });
        Level.setup(config[zone] || { });
    }
}
