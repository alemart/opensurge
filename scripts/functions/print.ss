// -----------------------------------------------------------------------------
// File: print.ss
// Description: a function object that prints a message
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

//
// Print is a function object that prints a message
//
object "Print"
{
    fun call(message)
    {
        Console.print(message);
    }
}