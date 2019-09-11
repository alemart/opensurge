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
            "Elevator": {
                "anim": 2
            },
            "Bridge": {
                "anim": 1
            },
            "Fish": {
                "anim": 1
            },
            "Audio Source": {
                "sound": "samples/waterfall.wav"
            },
            "Event Trigger 7": {
                "onTrigger": FunctionEvent("Lock Camera").withArgument(2048)
            }
        },

        //
        // zone 1 only
        //
        "1": {
            "Background Exchanger": {
                "background": "themes/waterworks_indoors.bg"
            },

            // Bridge
            "7af32f24d4d3fbad": {
                "layer": "yellow"
            },

            // Switch
            "b19e8c2134008c5a": {
                "onActivate": EventList([
                    EntityEvent("4df9ba74277932dd").willCall("open"), // open Door
                    FunctionEvent("Change Water Level").withArgument(9999999),
                    EntityEvent("Fish").willCall("destroy"),
                    EntityEvent("Skaterbug").willCall("destroy")
                ])
            }
        },

        //
        // zone 2 only
        //
        "2": {
            "Background Exchanger": {
                "background": "themes/waterworks.bg"
            },

            // Bridge
            "48bacb13be3c2c0f": {
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
