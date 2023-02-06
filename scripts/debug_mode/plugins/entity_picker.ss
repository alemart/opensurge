// -----------------------------------------------------------------------------
// File: entity_picker.ss
// Description: entity picker plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*

In order to interact with this Entity Picker, register your plugin as one of
its listeners and implement method onPickEntity(entityName) as in this example:

object "Debug Mode - My Entity Picker Test" is "debug-mode-plugin"
{
    debugMode = parent;

    state "main"
    {
        // initialization:
        // get the Entity Picker and register this plugin as a listener
        entityPicker = debugMode.plugin("Debug Mode - Entity Picker");
        entityPicker.subscribe(this);

        // done
        state = "waiting";
    }

    state "waiting"
    {
    }

    // this function will be called whenever the user picks an entity
    fun onPickEntity(entityName)
    {
        Console.print("Picked entity " + entityName);
    }
}

*/
using SurgeEngine.Actor;

object "Debug Mode - Entity Picker" is "debug-mode-plugin", "detached", "private", "entity"
{
    debugMode = parent;
    carousel = spawn("Debug Mode - Carousel");
    listeners = [];

    state "main"
    {
        // initialize the carousel
        carousel
            .add(entity("Collectible"))
            .add(entity("Checkpoint"))
            .add(entity("Goal"))
            .add(entity("Goal Capsule"))
            .add(entity("Bumper"))
            .add(entity("Layer Green"))
            .add(entity("Layer Yellow"))
            .add(entity("Powerup 1up"))
            .add(entity("Powerup Collectibles"))
            .add(entity("Powerup Lucky Bonus"))
            .add(entity("Powerup Invincibility"))
            .add(entity("Powerup Speed"))
            .add(entity("Powerup Shield"))
            .add(entity("Powerup Shield Acid"))
            .add(entity("Powerup Shield Fire"))
            .add(entity("Powerup Shield Thunder"))
            .add(entity("Powerup Shield Water"))
            .add(entity("Powerup Shield Wind"))
            .add(entity("Powerup Trap"))
            .add(entity("Spikes"))
            .add(entity("Spikes Down"))
            .add(entity("Spring Booster Left"))
            .add(entity("Spring Booster Right"))
            .add(entity("Water Bubbles"))
            .add(entity("Tube In"))
            .add(entity("Tube Out"))
        ;

        // done!
        state = "enabled";
    }

    state "enabled"
    {
        // do nothing
    }

    fun entity(entityName)
    {
        return spawn("Debug Mode - Entity Picker - Carousel Item Builder").setEntityName(entityName);
    }

    fun onCarouselItemPick(pickedItem)
    {
        foreach(item in carousel) {
            item.highlighted = false;
        }

        pickedItem.highlighted = true;
        _notify(pickedItem.name);
    }

    fun subscribe(listener)
    {
        if(listeners.indexOf(listener) < 0)
            listeners.push(listener);
    }

    fun _notify(entityName)
    {
        foreach(listener in listeners)
            listener.onPickEntity(entityName);
    }
}

object "Debug Mode - Entity Picker - Carousel Item Builder"
{
    entityName = "";

    fun build(itemContainer)
    {
        return itemContainer.spawn("Debug Mode - Entity Picker - Carousel Item").setEntityName(entityName);
    }

    fun setEntityName(name)
    {
        entityName = name;
        return this;
    }
}

object "Debug Mode - Entity Picker - Carousel Item" is "detached", "private", "entity"
{
    name = "";
    actor = null;



    fun setEntityName(entityName)
    {
        name = entityName;
        actor = Actor(name);

        assert(actor.animation.exists);
        actor.offset = actor.hotSpot;

        this.highlighted = false;
        return this;
    }

    fun get_name()
    {
        return name;
    }

    fun get_highlighted()
    {
        return actor.alpha == 1.0;
    }

    fun set_highlighted(highlighted)
    {
        actor.alpha = highlighted ? 1.0 : 0.5;
    }



    // the following methods are required for all Carousel items
    // the Carousel component will use these to adjust the UI
    fun get_naturalWidth()
    {
        return actor.width;
    }

    fun get_naturalHeight()
    {
        return actor.height;
    }

    fun get_zindex()
    {
        return actor.zindex;
    }

    fun set_zindex(zindex)
    {
        actor.zindex = zindex;
    }
}