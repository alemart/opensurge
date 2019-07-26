// -----------------------------------------------------------------------------
// File: unlock_camera.ss
// Description: a function object that unlocks the camera
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;

//
// Unlock Camera is a function object that
// unlocks the camera if previously locked
//
object "Unlock Camera"
{
    fun call()
    {
        camlock = Level.child("Camera Locker") || Level.spawn("Camera Locker");
        camlock.unlock();
    }
}