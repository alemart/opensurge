// -----------------------------------------------------------------------------
// File: undo_clear_level.ss
// Description: a function object that disables the cleared state of the level
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;

//
// Undo Clear Level is a function object that disables the cleared state of the
// level
//
object "Undo Clear Level"
{
    fun call()
    {
        Level.undoClear();
    }
}