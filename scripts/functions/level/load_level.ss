// -----------------------------------------------------------------------------
// File: level_level.ss
// Description: a function object that loads a level
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;

//
// Load Level is a function object that loads
// a specific level.
//
// Arguments:
// - path: string. The path of the .lev file to be loaded
//                 (e.g., "levels/template.lev")
//
object "Load Level"
{
    fun call(path)
    {
        Level.load(path);
    }
}