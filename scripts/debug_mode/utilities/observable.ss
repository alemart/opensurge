// -----------------------------------------------------------------------------
// File: observable.ss
// Description: Observable Pattern (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

object "Debug Mode - Observable" is "debug-mode-utility"
{
    observers = [];

    fun subscribe(observer)
    {
        if(observers.indexOf(observer) < 0)
            observers.push(observer);
    }

    fun unsubscribe(observer)
    {
        if((j = observers.indexOf(observer)) < 0)
            return;

        if(observers.length > 1)
            observers[j] = observers.pop(); // length -= 1
        else
            observers.clear();
    }

    fun notify(data)
    {
        length = observers.length;
        for(i = 0; i < length; i++)
            parent.onNotifyObserver(observers[i], data);
    }

    fun constructor()
    {
        if(!parent.hasTag("debug-mode-observable"))
            Console.print("Not tagged debug-mode-observable: " + parent.__name);
    }
}