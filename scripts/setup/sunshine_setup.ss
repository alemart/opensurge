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
            "Audio Source": {
                "sound": "samples/waterfall.wav"
            },
            "Bridge": {
                "anim": 0
            },
            "Fish": {
                "anim": 0
            },
            "Mosquito": {
                "anim": 0
            },
            "Zipline Grabber": {
                "anim": 1
            },
            "Event Trigger 7": {
                "onTrigger": FunctionEvent("Lock Camera").withArgument(2048)
            }
        },

        //
        // zone 1 only
        //
        "1": {
            // Bridge
            "75d7cacbe3448333": {
                "length": 8,
                "layer": "green"
            },

            // Bridge
            "ce6187aed9016033": {
                "length": 8
            },

            // Bridge
            "1706a5edf48b0e33": {
                "length": 8
            },

            // Bridge
            "9f41cc898da84f8b": {
                "length": 10
            },

            // Bridge
            "6160b07a29ec8598": {
                "length": 14
            },

            // Bridge
            "a8f66404e8ca6d3c": {
                "length": 14
            }
        },

        //
        // zone 2 only
        //
        "2": {
            // Bridge
            "15f489476699f7d5": {
                "length": 14
            },

            // Bridge
            "6a824b81a2f4ad4f": {
                "length": 18
            },

            // Bridge
            "85ac1895bc891dd1": {
                "length": 14
            },

            // Bridge
            "decfa93bc1602d3f": {
                "length": 20
            }
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
