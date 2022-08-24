// -----------------------------------------------------------------------------
// File: waterworks-zone.ss
// Description: setup object for Waterworks Zone
// Authors: Alexandre Martins, Cody Licorish <http://opensurge2d.org>
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

object "Waterworks Zone" is "setup"
{
    config = {

        //
        // all zones / acts
        //
        "*": {
            "Elevator": {
                "anim": 3
            },
            "Jumping Fish": {
                "anim": 1
            },
            "Audio Source": {
                "sound": "samples/waterfall.wav"
            },
            "Conveyor Belt A": {
                "speed": -120,
                "fps": 30
            },
            "Conveyor Belt B": {
                "speed": +120,
                "fps": 30
            },
            "Animals": {
                "theme": [0, 1, 2, 4, 5, 6, 8, 8, 11, 12]
            },
            "Event Trigger 7": {
                "onTrigger": FunctionEvent("Lock Camera").withArgument(2048)
            },
        },

        //
        // zone 1 only
        //
        "1": {
            "Background Exchanger": {
                "background": "themes/waterworks-zone-boss.bg"
            },
            "Bridge": {
                "anim": 0
            },
        },

        //
        // zone 2 only
        //
        "2": {
            "Bridge": {
                "length": 4,
                "layer": "yellow",
                "anim": 1
            },
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
