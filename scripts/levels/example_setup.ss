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

            // Example: configuring all bridges
            "Bridge": {
                "length": 12,
                "anim": 1
            },

            // Example: configuring all elevators
            "Elevator": {
                "anim": 2 // animation number
            },

            // Example: configuring an event
            "Event Trigger 1": {
                "onTrigger": FunctionEvent("Print").addParameter("Hello from Event 1!")
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

            // Example: collapse a specific bridge (id: 481c9ccb42a38268)
            // when activating a specific switch (id: 142f0aa6855b991a).
            "142f0aa6855b991a": {
                "onActivate": EntityEvent("481c9ccb42a38268", "collapse")
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

    //
    // Below you'll find the setup code.
    // You can keep the defaults or extend it.
    //
    state "main"
    {
        zone = String(Level.act);

        // setup the entities
        Level.setup(config["*"] || { });
        Level.setup(config[zone] || { });

        // done
        state = "done";
    }

    state "done"
    {
        // the setup is done!
    }
}