// -----------------------------------------------------------------------------
// File: print.ss
// Description: a function object that prints a message
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

//
// Print is a function object that prints a message
//
// Arguments:
// - message: string. The message to be printed.
//
object "Print"
{
    fun call(message)
    {
        Console.print(message);
    }
}