// -----------------------------------------------------------------------------
// File: video_changer.ss
// Description: video changer plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*

This plugin changes the video mode of the engine in Desktop computers.

*/

using SurgeEngine;
using SurgeEngine.Video;

object "Debug Mode - Video Changer" is "debug-mode-plugin"
{
    previousVideoMode = "";

    fun onLoad(debugMode)
    {
        if(SurgeEngine.mobile)
            return;

        previousVideoMode = Video.mode;
        Video.mode = "fill";
    }

    fun onUnload(debugMode)
    {
        if(SurgeEngine.mobile)
            return;

        Video.mode = previousVideoMode;
    }
}