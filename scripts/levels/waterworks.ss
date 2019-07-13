// -----------------------------------------------------------------------------
// File: waterworks.ss
// Description: startup object for Waterworks Zone
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;

object "Waterworks Setup"
{
    state "main"
    {
        // setup properties
        Level.setup({
            "Elevator": {
                "anim": 2
            },
            "Background Exchanger": {
                "background": "themes/template.bg"
            }
        });

        // done
        state = "done";
    }

    state "done"
    {
    }
}