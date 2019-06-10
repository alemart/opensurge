// -----------------------------------------------------------------------------
// File: waterworks.ss
// Description: utilities for Waterworks Zone
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Level;
using SurgeEngine.Transform;
using SurgeEngine.Vector2;

// startup object
object "WaterworksSetup"
{
    state "main"
    {
        // configure elevators
        elevators = Level.findObjects("Elevator");
        foreach(elevator in elevators)
            elevator.anim = 2;

        // configure background exchangers
        xchgs = Level.findObjects("Background Exchanger");
        foreach(xchg in xchgs)
            xchg.background = "themes/template.bg";

        // done
        state = "done";
    }

    state "done"
    {
    }
}