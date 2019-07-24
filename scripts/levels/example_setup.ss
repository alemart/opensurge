// -----------------------------------------------------------------------------
// File: example_setup.ss
// Description: example setup object
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;
using SurgeEngine.Events.EventList;
using SurgeEngine.Events.EntityEvent;
using SurgeEngine.Events.FunctionEvent;

object "Example Setup"
{
    //
    // This is the config object.
    // Use it to configure your entities.
    //
    config = {

        //
        // all zones / acts
        //
        "*": {
            // the configuration below is just an example
            // (modify at will - tip: backup this file first)

            // Example: configuring all Bridges
            "Bridge": {
                "length": 12, // length 12 is the default
                "anim": 1
            },

            // Example: configuring all Elevators
            "Elevator": {
                "anim": 2 // animation number
            },

            // Example: configuring an event
            "Event Trigger 1": {
                "onTrigger": FunctionEvent("Print").withArgument("Hello! Give me pizza!")
            }
        },

        //
        // zone 1 only
        //
        "1": {
            // Example: configuring the Background Exchanger for zone 1
            "Background Exchanger": {
                "background": "themes/template.bg"
            },

            // Example: collapse a specific Bridge (id: 481c9ccb42a38268)
            // when activating a specific Switch (id: 142f0aa6855b991a)
            "142f0aa6855b991a": {
                "onActivate": EntityEvent("481c9ccb42a38268").willCall("collapse")
            },

            // Example: open a specific Door (id: 2bbc752e0454d031) ONLY
            // IF a specific red Switch (id: 66b9b90da2a5a5fa) is pressed
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
            // Make them trigger multiple events!
            "Switch": {
                "color": "blue",
                "onActivate": EventList([
                    FunctionEvent("Print").withArgument("Oh my! Look at the water!"),
                    FunctionEvent("Water Level").withArgument(128) // raise water to ypos = 128
                ])
            }
        },

        //
        // zone 3 only
        //
        "3": {

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