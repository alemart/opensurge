// -----------------------------------------------------------------------------
// File: item_picker.ss
// Description: item picker plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*

In order to interact with this Item Picker, register your plugin as one of
its listeners and implement method onPickItem(item) as in this example:

object "Debug Mode - My Item Picker Test" is "debug-mode-plugin"
{
    debugMode = parent;

    state "main"
    {
        // initialization:
        // get the Item Picker and register this plugin as a listener
        itemPicker = debugMode.plugin("Debug Mode - Item Picker");
        itemPicker.subscribe(this);

        // done
        state = "waiting";
    }

    state "waiting"
    {
    }

    // this function will be called whenever the user picks an item
    fun onPickItem(item)
    {
        Console.print("Picked item " + item.name);
        Console.print(item.type);
    }
}

*/
using SurgeEngine.Actor;

object "Debug Mode - Item Picker" is "debug-mode-plugin", "detached", "private", "entity"
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
            .add(entity("Spikes"))
            .add(entity("Spikes Down"))
            .add(entity("Bridge"))
            .add(entity("Water Bubbles"))
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
            .add(entity("Spring Booster Left"))
            .add(entity("Spring Booster Right"))
            .add(entity("Spring Booster Up"))
            .add(entity("Spring Standard"))
            .add(entity("Spring Standard Down"))
            .add(entity("Spring Standard Down Left"))
            .add(entity("Spring Standard Down Right"))
            .add(entity("Spring Standard Hidden"))
            .add(entity("Spring Standard Left"))
            .add(entity("Spring Standard Right"))
            .add(entity("Spring Standard Up Left"))
            .add(entity("Spring Standard Up Right"))
            .add(entity("Spring Stronger"))
            .add(entity("Spring Stronger Down"))
            .add(entity("Spring Stronger Down Left"))
            .add(entity("Spring Stronger Down Right"))
            .add(entity("Spring Stronger Hidden"))
            .add(entity("Spring Stronger Left"))
            .add(entity("Spring Stronger Right"))
            .add(entity("Spring Stronger Up Left"))
            .add(entity("Spring Stronger Up Right"))
            .add(entity("Spring Strongest"))
            .add(entity("Spring Strongest Down"))
            .add(entity("Spring Strongest Down Left"))
            .add(entity("Spring Strongest Down Right"))
            .add(entity("Spring Strongest Hidden"))
            .add(entity("Spring Strongest Left"))
            .add(entity("Spring Strongest Right"))
            .add(entity("Spring Strongest Up Left"))
            .add(entity("Spring Strongest Up Right"))
            .add(entity("Zipline Grabber"))
            .add(entity("Zipline Left"))
            .add(entity("Zipline Right"))
            .add(entity("Power Pluggy Clockwise"))
            .add(entity("Power Pluggy Counterclockwise"))
            .add(entity("Lady Bugsy"))
            .add(entity("Springfling"))
            .add(entity("Wolfey"))
            .add(entity("Mosquito"))
            .add(entity("Jumping Fish"))
            .add(entity("Crococopter"))
            .add(entity("GreenMarmot"))
            .add(entity("RedMarmot"))
            .add(entity("RulerSalamander"))
            .add(entity("SwoopHarrier"))
            .add(entity("Skaterbug"))
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
        return spawn("Debug Mode - Item Picker - Carousel Item Builder - Entity").setEntityName(entityName);
    }

    fun onPickCarouselItem(pickedCarouselItem)
    {
        // highlight picked item
        foreach(item in carousel) {
            item.highlighted = false;
        }
        pickedCarouselItem.highlighted = true;

        // notify listeners
        item = Item(pickedCarouselItem.type, pickedCarouselItem.name);
        _notify(item);
    }

    fun Item(type, name)
    {
        return spawn("Debug Mode - Item Picker - Item").init(type, name);
    }

    fun subscribe(listener)
    {
        if(listeners.indexOf(listener) < 0)
            listeners.push(listener);
    }

    fun _notify(item)
    {
        foreach(listener in listeners)
            listener.onPickItem(item);
    }
}

object "Debug Mode - Item Picker - Item"
{
    type = "";
    name = "";

    fun get_type()
    {
        return type;
    }

    fun get_name()
    {
        return name;
    }

    fun init(itemType, itemName)
    {
        type = itemType;
        name = itemName;
        return this;
    }
}



//
// Item type: entity
//

object "Debug Mode - Item Picker - Carousel Item Builder - Entity"
{
    entityName = "";

    fun build(itemContainer)
    {
        return itemContainer.spawn("Debug Mode - Item Picker - Carousel Item - Entity").setEntityName(entityName);
    }

    fun setEntityName(name)
    {
        entityName = name;
        return this;
    }
}

object "Debug Mode - Item Picker - Carousel Item - Entity" is "detached", "private", "entity"
{
    public readonly type = "entity";
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