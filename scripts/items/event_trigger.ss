// -----------------------------------------------------------------------------
// File: event_triggers.ss
// Description: script of the event triggers
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;
using SurgeEngine.Events.Event;
using SurgeEngine.Collisions.CollisionBox;

//
//            EVENT TRIGGERS
//              HOW TO USE
//
// When the player touches an Event Trigger
// object, a user-defined event will be
// triggered.
//
// There are different trigger groups (1, 2,
// 3, 4...) - every Event Trigger object
// belongs to exactly one group.
//
// When an Event Trigger is touched, all other
// objects belonging to the same group will no
// longer be able to be triggered*. Only the
// touched object will trigger the event. It
// happens once, unless the level restarts
// (* unless you reactivate the group!)
//
// To define the specific events, you may use
// a level setup script that runs on the level
// setup. Example:
//
// using SurgeEngine.Level;
// using SurgeEngine.Events.FunctionEvent;
//
// /* in your setup script... */
// Level.setup({
//   "Event Trigger 1": {
//     "onTrigger": FunctionEvent("Print").withArgument("Hello from Event!")
//   }
// });
//
// The example above will call function object
// "Print" with parameter "Hello from Event!"
// when the player first touches a group 1
// Event Trigger.
//
// If you want to reactivate an Event Trigger,
// so that it can be triggered multiple times,
// use a DelayedEvent in an EventList. Example:
//
// using SurgeEngine.Level;
// using SurgeEngine.Events.EventList;
// using SurgeEngine.Events.EntityEvent;
// using SurgeEngine.Events.DelayedEvent;
// using SurgeEngine.Events.FunctionEvent;
//
// /* in your setup script... */
// Level.setup({
//   "Event Trigger 1": {
//     "onTrigger": EventList([
//       FunctionEvent("Print").withArgument("Hello from Event!"),
//       DelayedEvent(
//         EntityEvent("Event Trigger 1").willCall("reactivate")
//       ).willWait(2.0)
//     ])
//   }
// });
//

// Event Trigger 1
object "Event Trigger 1" is "entity", "special"
{
    public onTrigger = Event();
    base = spawn("Event Trigger Base").setGroup(1);

    fun trigger()
    {
        onTrigger();
    }

    fun reactivate()
    {
        base.reactivate(); // delegate
    }
}

// Event Trigger 2
object "Event Trigger 2" is "entity", "special"
{
    public onTrigger = Event();
    base = spawn("Event Trigger Base").setGroup(2);

    fun trigger()
    {
        onTrigger();
    }

    fun reactivate()
    {
        base.reactivate();
    }
}

// Event Trigger 3
object "Event Trigger 3" is "entity", "special"
{
    public onTrigger = Event();
    base = spawn("Event Trigger Base").setGroup(3);

    fun trigger()
    {
        onTrigger();
    }

    fun reactivate()
    {
        base.reactivate();
    }
}

// Event Trigger 4
object "Event Trigger 4" is "entity", "special"
{
    public onTrigger = Event();
    base = spawn("Event Trigger Base").setGroup(4);

    fun trigger()
    {
        onTrigger();
    }

    fun reactivate()
    {
        base.reactivate();
    }
}

// Event Trigger 5
object "Event Trigger 5" is "entity", "special"
{
    public onTrigger = Event();
    base = spawn("Event Trigger Base").setGroup(5);

    fun trigger()
    {
        onTrigger();
    }

    fun reactivate()
    {
        base.reactivate();
    }
}

// Event Trigger 6
object "Event Trigger 6" is "entity", "special"
{
    public onTrigger = Event();
    base = spawn("Event Trigger Base").setGroup(6);

    fun trigger()
    {
        onTrigger();
    }

    fun reactivate()
    {
        base.reactivate();
    }
}

// Event Trigger 7
object "Event Trigger 7" is "entity", "special"
{
    public onTrigger = Event();
    base = spawn("Event Trigger Base").setGroup(7);

    fun trigger()
    {
        onTrigger();
    }

    fun reactivate()
    {
        base.reactivate();
    }
}

// Event Trigger 8
object "Event Trigger 8" is "entity", "special"
{
    public onTrigger = Event();
    base = spawn("Event Trigger Base").setGroup(8);

    fun trigger()
    {
        onTrigger();
    }

    fun reactivate()
    {
        base.reactivate();
    }
}

//  --------------------

object "Event Trigger Base" is "private", "entity", "special"
{
    collider = CollisionBox(32, 32);
    manager = Level.child("Event Trigger Manager") || Level.spawn("Event Trigger Manager");
    group = 0;

    state "main"
    {
    }

    fun setGroup(groupId)
    {
        group = Number(groupId);
        return this;
    }

    fun reactivate()
    {
        manager.reactivate(group);
    }

    fun onCollision(otherCollider)
    {
        if(otherCollider.entity.hasTag("player") && !manager.triggered(group)) {
            player = otherCollider.entity;
            manager.trigger(group, player);
            parent.trigger();
        }
    }
}

object "Event Trigger Manager"
{
    triggers = [ null, null, null, null, null, null, null, null, null ];

    state "main"
    {
    }

    fun triggered(groupId)
    {
        if(groupId >= 0 && groupId < triggers.length)
            return triggers[groupId] !== null;
        else
            return false;
    }

    fun triggerer(groupId)
    {
        if(groupId >= 0 && groupId < triggers.length)
            return triggers[groupId];
        else
            return null;
    }

    fun trigger(groupId, player)
    {
        if(groupId >= 0 && groupId < triggers.length)
            triggers[groupId] = player.name;
    }

    fun reactivate(groupId)
    {
        if(groupId >= 0 && groupId < triggers.length)
            triggers[groupId] = null;
    }
}