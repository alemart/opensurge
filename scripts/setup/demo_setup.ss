// -----------------------------------------------------------------------------
// File: demo_setup.ss
// Description: setup object for the Demo Level
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine;
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
using SurgeTheRabbit;

object "Demo Setup"
{
    config = {

        //
        // all zones / acts
        //
        "*": {
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
            "Event Trigger 1": {
                "onTrigger": FunctionEvent("Show Message").withArgument(Lang["LEV_DEMO_1"])
            },
            "Event Trigger 2": {
                "onTrigger": FunctionEvent("Show Message").withArgument(Lang["LEV_DEMO_2"])
            },
            "Event Trigger 3": {
                "onTrigger": FunctionEvent("Show Message").withArgument(Lang["LEV_DEMO_3"])
            },
            "Event Trigger 4": {
                "onTrigger": FunctionEvent("Show Message").withArgument(Lang["LEV_DEMO_4"])
            },
            "Bridge": {
                "anim": 0
            },
            "Zipline Grabber": {
                "anim": 1
            }
        },

        //
        // zone 2 only
        //
        "2": {
            "Event Trigger 1": {
                "onTrigger": FunctionEvent("Show Message").withArgument(
                    !SurgeEngine.mobile ? Lang["LEV_DEMO_5"] : Lang["LEV_DEMO_5_MOBILE"]
                )
            },
            "Event Trigger 2": {
                "onTrigger": EventList([
                    DelayedEvent(
                        FunctionEvent("Show Message")
                        .withArgument(Lang["LEV_DEMO_6"])
                    ).willWait(7.5),
                    DelayedEvent(
                        FunctionEvent("Invoke")
                        .withArgument(SurgeTheRabbit)
                        .withArgument("visitHomepage")
                        .withArgument([])
                    ).willWait(15.0)
                ])
            },
            "Bridge": {
                "anim": 1
            },
            "Zipline Grabber": {
                "anim": 0
            },
            "Jumping Fish": {
                "anim": 1
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
