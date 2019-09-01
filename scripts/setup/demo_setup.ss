// -----------------------------------------------------------------------------
// File: demo_setup.ss
// Description: setup object for the Demo Level
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;
using SurgeEngine.Events.EventList;
using SurgeEngine.Events.EntityEvent;
using SurgeEngine.Events.FunctionEvent;

object "Demo Setup"
{
    config = {

        //
        // all zones / acts
        //
        "*": {
            "Bridge": {
                "anim": 0
            },
            "Audio Source": {
                "sound": "samples/waterfall.wav"
            },
            "Zipline Grabber": {
                "anim": 1
            },
        },

        //
        // zone 1 only
        //
        "1": {

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
