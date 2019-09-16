// -----------------------------------------------------------------------------
// File: example_setup.ss
// Description: example setup object
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

/*
                      SETUP SCRIPT

The setup script is used to set up entities and data in a level.
Below you'll find a configuration object partitioned into zones.

Each zone is likely to contain multiple entities. Entities have
a name (like "Bridge", "Elevator", and so on) and an ID (a hex
code like "2bbc752e0454d031"). While multiple entities may have
the same name (e.g., there may be multiple bridges on a level),
each entity is uniquely identified by its ID.

Entities may be referenced in the configuration object using
their names or their unique IDs. Once you reference an entity,
you may change its properties following the template below.
The exact properties of an entity can be found by inspecting
it in the level editor.

To associate a setup script with a particular level, open the
.lev file with a text editor and add the name of the setup
object (e.g., "Example Setup") to the setup field. Example:

    setup "Default Setup" "Example Setup"

Happy hacking! ;)

Tip: explore folder scripts/functions/ to know about many
     functions you can use with events.
*/

object "Example Setup"
{
    //
    // This is the configuration object.
    // Use it to configure your entities.
    //
    config = {

        //
        // all zones / acts
        //
        "*": {
            // the configuration below is just an example
            // (modify at will, but backup this file first!)

            // Example: configuring all Bridges. Setting their
            // length and their animation number.
            "Bridge": {
                "length": 12, // length 12 is the default
                "anim": 1     // animation number (the sprite is "Bridge")
            },

            // Example: configuring all Elevators.
            "Elevator": {
                "anim": 2 // animation number
            },

            // Example: configuring all Audio Sources.
            "Audio Source": {
                "sound": "samples/waterfall.wav" // path to a sound
            }
        },

        //
        // zone 1 only
        //
        "1": {
            // Example: configuring the Background Exchanger for zone 1.
            "Background Exchanger": {
                "background": "themes/template.bg"
            },

            // Example: making a specific Door become red.
            // (suppose its ID is 532ab77cef1252ae)
            "532ab77cef1252ae": {
                "color": "red"
            },

            // Example: configuring an event. An event is something that
            // may be triggered during gameplay. This will print a message
            // as soon as the player touches an Event Trigger 1 entity.
            "Event Trigger 1": {
                "onTrigger": FunctionEvent("Print").withArgument("Hello! Give me pizza!")
            },

            // Example: lock the camera as soon as the player touches an
            // Event Trigger 7 entity, ideally placed near the goal flag.
            "Event Trigger 7": {
                "onTrigger": FunctionEvent("Lock Camera").withArgument(2048) // give a maximum space of 2048 pixels to the right
            },

            // Example: collapse a specific Bridge (ID: 481c9ccb42a38268)
            // when activating a specific Switch (ID: 142f0aa6855b991a).
            "142f0aa6855b991a": {
                "onActivate": EntityEvent("481c9ccb42a38268").willCall("collapse")
            },

            // Example: open a specific Door (ID: 2bbc752e0454d031) ONLY IF
            // a specific red Switch (ID: 66b9b90da2a5a5fa) is being pressed.
            "66b9b90da2a5a5fa": {
                "color": "red",
                "sticky": false,
                "onActivate": EntityEvent("2bbc752e0454d031").willCall("open"),
                "onDeactivate": EntityEvent("2bbc752e0454d031").willCall("close")
            }
        },

        //
        // zone 2 only
        //
        "2": {
            // Example: make all Switches blue.
            // Make them trigger multiple events at the same time!
            "Switch": {
                "color": "blue",
                "onActivate": EventList([
                    FunctionEvent("Print").withArgument("Oh my! Look at the water!"),
                    FunctionEvent("Change Water Level").withArgument(128) // raise water to ypos = 128
                ])
            },

            // Example: tell a story. Trigger one event at a time!
            "Event Trigger 2": {
                "onTrigger": EventList([
                    FunctionEvent("Print").withArgument("Oh my! Something is about to happen!"),
                    DelayedEvent(
                        FunctionEvent("Print").withArgument("I can feel... water...")
                    ).willWait(3.0), // wait 3 seconds before triggering this
                    DelayedEvent(
                        FunctionEvent("Print").withArgument("WATCH OUT!")
                    ).willWait(6.0), // wait 6 seconds before triggering this
                    DelayedEvent(
                        FunctionEvent("Change Water Level").withArgument(128)
                    ).willWait(7.0) // wait 7 seconds before triggering this
                ])
            },

            // Example: trigger different events when pressing a
            // Switch multiple times. The events will be triggered
            // sequentially, like the links of a chain.
            "e58aecd5740a0547": {
                "sticky": false,
                "onActivate": EventChain([
                    FunctionEvent("Print").withArgument("First time you pressed it"),
                    FunctionEvent("Print").withArgument("Second time you pressed it"),
                    FunctionEvent("Print").withArgument("Third time you pressed it"),
                    FunctionEvent("Print").withArgument("Don't you get tired?"),
                    FunctionEvent("Print").withArgument("Enough!")
                ])
            },

            // Example: trigger alternating events. Raise the water
            // level and then disable it, again and again, whenever
            // the blue Switch is pressed.
            "ab5264eabc2564cc": {
                "color": "blue",
                "sticky": false,
                "onActivate": EventChain([
                    FunctionEvent("Change Water Level").withArgument(128),
                    FunctionEvent("Change Water Level").withArgument(9999999)
                ]).willLoop()
            }
        },

        //
        // zone 3 only
        //
        "3": {
            // Example: start a boss fight!
            "Event Trigger 7": {
                "onTrigger": EventList([
                    FunctionEvent("Play Boss Music"),
                    FunctionEvent("Lock Camera").withArgument(800) // lock camera with a space of 800 pixels to the right
                ])
            },

            // Example: extend the locked region after a boss fight.
            // "My Boss" is a hypothetical entity that triggers onDefeat.
            "My Boss": {
                "onDefeat": EventList([
                    FunctionEvent("Stop Boss Music"),
                    FunctionEvent("Lock Camera").withArgument(512) // give more 512 pixels of space
                ])
            },

            // Example: unlock the camera after a mini-boss fight,
            // so the player can keep moving throughout the level.
            "My Mini Boss": {
                "onDefeat": EventList([
                    FunctionEvent("Stop Boss Music"),
                    FunctionEvent("Unlock Camera")
                ])
            },

            // Example: when touching Event Trigger 3, give the player
            // a bonus of 50 collectibles to prepare for the boss fight!
            "Event Trigger 3": {
                "onTrigger": FunctionEvent("Give Lucky Bonus").withArgument(50)
            }
        }

    };

    // -------------------------------------------------------------------------

    //
    // Below you'll find the setup code.
    // You can keep it as it is or extend it.
    //
    state "main"
    {
        // write your code here
    }

    // setup the entities
    fun constructor()
    {
        zone = String(Level.act);
        Level.setup(config["*"] || { });
        Level.setup(config[zone] || { });
    }
}