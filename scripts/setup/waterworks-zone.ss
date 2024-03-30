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
            "Background Exchanger": {
                "background": "themes/waterworks-zone-outdoors.bg"
            },
            "Bridge": {
                "anim": 1,
            },
        },

        //
        // zone 3 only
        //
        "3": {
            "Event Trigger 7": {
                "onTrigger": EventList([
                    EntityEvent("Salamander Boss").willCall("activate"),
                    EntityEvent("Salamander Invisible Wall").willCall("activate"),
                    FunctionEvent("Play Boss Music")
                ])
            },
            "Event Trigger 5": {
                "onTrigger": EventList([
                    FunctionEvent("Translate Player").withArgument(Vector2(0,-3840)),
                    EntityEvent("Salamander Boss").willCall("translate").withArgument(Vector2(0,-3840)),
                    FunctionEvent("Translate Default Camera").withArgument(Vector2(0,-3840)),
                    DelayedEvent(
                        EntityEvent("Event Trigger 5").willCall("reactivate")
                    ).willWait(2.0),
                    EntityEvent("Collectible").willCall("destroy"),
                    DelayedEvent(
                      EventList([
                        FunctionEvent("Spawn Entity") // warning mask tool
                            .withArgument("Collectible").withArgument(Vector2(2880,700)),
                        FunctionEvent("Spawn Entity")
                            .withArgument("Collectible").withArgument(Vector2(2880,13776)),
                        FunctionEvent("Spawn Entity")
                            .withArgument("Collectible").withArgument(Vector2(2896,13808)),
                        FunctionEvent("Spawn Entity")
                            .withArgument("Collectible").withArgument(Vector2(2864,13808)),
                        FunctionEvent("Spawn Entity")
                            .withArgument("Collectible").withArgument(Vector2(3136,13776)),
                        FunctionEvent("Spawn Entity")
                            .withArgument("Collectible").withArgument(Vector2(3120,13808)),
                        FunctionEvent("Spawn Entity")
                            .withArgument("Collectible").withArgument(Vector2(3152,13808))
                      ])
                    ).willWait(0.25)
                ])
            },

            "Salamander Boss": {
                "onFirstFall": FunctionEvent("Lock Camera")
                    .withArgument([2816,9344,3200,20224]),
                "onDefeat": EventList([
                    FunctionEvent("Stop Boss Music"),
                    FunctionEvent("Unlock Camera"),
                    EntityEvent("Salamander Invisible Wall").willCall("deactivate"),
                    DelayedEvent(
                      EntityEvent("Salamander Exploding Wall").willCall("explode")
                    ).willWait(1.0)
                ]),
            },
            "Bridge": {
                "length": 24
            },
            "Background Exchanger": {
                "background": "themes/waterworks-zone-boss.bg"
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
