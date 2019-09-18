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
            "Jumping Fish": {
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
                "background": "themes/waterworks_indoors_fg.bg"
            },

            "Switch": {
                "onActivate": EventList([
                    FunctionEvent("Change Water Level").withArgument(9999999),
                    EntityEvent("Door").willCall("open"),
                    EntityEvent("Jumping Fish").willCall("destroy"),
                    EntityEvent("Skaterbug").willCall("destroy")
                ])
            },

            // Bridge
            "7af32f24d4d3fbad": {
                "layer": "yellow"
            },
        },

        //
        // zone 2 only
        //
        "2": {
            "Background Exchanger": {
                "background": "themes/waterworks.bg"
            },

            "Switch": {
                "onActivate": EntityEvent("Door").willCall("open")
            },

            "Event Trigger 1": {
                "onTrigger": EventList([
                    EntityEvent("Door").willCall("close"),
                    FunctionEvent("Change Water Level").withArgument(9728)
                ])
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
           "Event Trigger 7": {
                "onTrigger": EventList([
                    EntityEvent("Hydra").willCall("activate"),
                    FunctionEvent("Play Boss Music"),
                    FunctionEvent("Lock Camera").withArgument(640)
                ])
            },

            "Hydra": {
                "onDefeat": EventList([
                    FunctionEvent("Stop Boss Music"),
                    FunctionEvent("Lock Camera").withArgument(9999)
                ])
            },

            "Bridge": {
                "length": 24
            }
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
