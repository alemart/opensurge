// -----------------------------------------------------------------------------
// File: clear_level.ss
// Description: a function object that clears the level
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;

//
// Clear Level is a function object that clears
// the current level.
//
object "Clear Level"
{
    fun call()
    {
        Level.clear();
    }
}