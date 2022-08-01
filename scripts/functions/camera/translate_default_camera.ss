// -----------------------------------------------------------------------------
// File: translate_default_camera.ss
// Description: a function object that translates the Default Camera
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;

//
// Translate Default Camera is a function object that translates the Default
// Camera by a specified offset.
//
// Arguments:
// - offset: Vector2.
//
object "Translate Default Camera"
{
    camera = null;

    fun call(offset)
    {
        // find the Default Camera
        if(camera === null) {
            camera = Level.findObject("Default Camera");
            if(camera === null) {
                Console.print("Can't find the Default Camera");
                return;
            }
        }

        // translate & refresh
        camera.translate(offset);
        camera.refresh();
    }
}