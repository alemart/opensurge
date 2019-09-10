// -----------------------------------------------------------------------------
// File: waterworks_setup.ss
// Description: setup object for Waterworks Zone
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

object "Waterworks Setup"
{
    config = {

        //
        // all zones / acts
        //
        "*": {
            "Background Exchanger": {
                "background": "themes/waterworks_indoors.bg"
            },
            "Elevator": {
                "anim": 2
            },
            "Bridge": {
                "anim": 1
            },
            "Audio Source": {
                "sound": "samples/waterfall.wav"
            }
        },

        //
        // zone 1 only
        //
        "1": {
            // Bridge
            "7af32f24d4d3fbad": {
                "layer": "yellow"
            }
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
