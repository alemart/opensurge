// -----------------------------------------------------------------------------
// File: invoke.ss
// Description: a function object that invokes a function of another object
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

//
// Invoke is a function object that invokes a function of another object. This
// is particularly useful when combined with a FunctionEvent (see below).
//
// Arguments:
// - obj: object. The target object.
// - method: string. The name of the function to be invoked.
// - params: array. The parameters to be passed to the function. It may be empty.
//
// Example:
//
// using SurgeEngine.Events.FunctionEvent;
// using SurgeEngine.Web;
// ...
// fn = FunctionEvent("Invoke")
//      .withArgument(Web)
//      .withArgument("launchURL")
//      .withArgument([ "http://opensurge2d.org" ]);
//
// Calling fn() is equivalent to calling Web.launchURL("http://opensurge2d.org").
// The difference is that fn may be passed as a parameter to other functions!
// This technique lets you store a function call in a variable. In addition,
// you can wrap similar function events in an EventList to store multiple calls.
//
// Note that events do not return a value, whereas Invoke returns the returned
// value of the invoked function, or null on error. A wrapper can be created to
// get the returned value of the invoked function.
//

object "Invoke"
{
    fun call(obj, method, params)
    {
        if(obj === null)
            Console.print("Invoke: target object is null");
        else if(typeof obj != "object")
            Console.print("Invoke: " + obj.toString() + " is not an object");
        else if(!obj.hasFunction(method))
            Console.print("Invoke: " + obj.__name + "." + method + "() doesn't exist");
        else if(typeof params != "object" || params.__name != "Array")
            Console.print("Invoke: an array of parameters is expected to call " + obj.__name + "." + method + "()");
        else if(obj.__arity(method) != params.length)
            Console.print("Invoke: " + obj.__name + "." + method + "() expects " + obj.__arity(method) + " parameters, not " + params.length);
        else
            return obj.__invoke(method, params);

        return null;
    }
}
