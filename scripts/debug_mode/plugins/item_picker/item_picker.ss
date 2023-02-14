// -----------------------------------------------------------------------------
// File: item_picker.ss
// Description: item picker plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*

This plugin lets users pick items in a carousel.

In order to interact with this Item Picker, register your plugin as one of
its observers and implement method onPickItem(item) as in this example:

object "Debug Mode - My Item Picker Test" is "debug-mode-plugin"
{
    fun onLoad(debugMode)
    {
        itemPicker = debugMode.plugin("Debug Mode - Item Picker");
        itemPicker.subscribe(this);
    }

    fun onUnload(debugMode)
    {
        itemPicker = debugMode.plugin("Debug Mode - Item Picker");
        itemPicker.unsubscribe(this);
    }

    fun onPickItem(item)
    {
        Console.print("Picked item " + item.name);
        Console.print(item.type);
    }
}

*/

using SurgeEngine.Actor;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;

// Item Picker
object "Debug Mode - Item Picker" is "debug-mode-plugin", "debug-mode-observable", "detached", "private", "entity"
{
    transform = Transform();
    stack = spawn("Debug Mode - Item Picker - Stack");
    observable = spawn("Debug Mode - Observable");

    fun add(itemBuilder)
    {
        stack.top.add(itemBuilder);
        return this;
    }

    /*
    fun clear()
    {
        stack.top.clear();
        return this;
    }
    */

    fun push()
    {
        stack.push();
        return this;
    }

    fun pop()
    {
        stack.pop();
        return this;
    }

    fun onPickCarouselItem(pickedCarouselItem)
    {
        // notify observers
        item = Item(pickedCarouselItem.type, pickedCarouselItem.name);
        observable.notify(item);
    }

    fun Item(type, name)
    {
        return spawn("Debug Mode - Item Picker - Item").init(type, name);
    }

    fun subscribe(observer)
    {
        observable.subscribe(observer);
    }

    fun unsubscribe(observer)
    {
        observable.unsubscribe(observer);
    }

    fun onNotifyObserver(observer, item)
    {
        observer.onPickItem(item);
    }

    fun constructor()
    {
        transform.position = Vector2.zero;
    }
}

// an item that is picked
object "Debug Mode - Item Picker - Item"
{
    public readonly picker = parent;
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

// a stack of groups of items
object "Debug Mode - Item Picker - Stack" is "detached", "private", "entity"
{
    stack = []; // we require stack.length >= 1 (root) - see the constructor below
    pool = [];
    poolSize = 16;
    poolPointer = 0;

    state "main" {}
    state "frozen" { state = "main"; }

    fun get_top()
    {
        // top of the stack
        return stack[stack.length - 1];
    }

    fun push()
    {
        if(poolPointer >= poolSize) {
            Console.print("Exhausted pool in " + this.__name);
            return;
        }

        carousel = pool[poolPointer++];
        carousel.clear();

        this.top.transform.localPosition = Vector2(0, -carousel.height);
        carousel.transform.localPosition = Vector2.zero;

        stack.push(carousel);
        state = "frozen";
    }

    fun pop()
    {
        if(stack.length <= 1) {
            Console.print("Can't empty stack in " + this.__name);
            return;
        }

        carousel = stack.pop();
        --poolPointer;

        carousel.transform.localPosition = Vector2(0, -carousel.height);
        this.top.transform.localPosition = Vector2.zero;

        state = "frozen";
    }

    fun onPickCarouselItem(pickedCarouselItem)
    {
        if(state == "frozen")
            return;

        parent.onPickCarouselItem(pickedCarouselItem);
    }

    fun constructor()
    {
        for(i = 0; i < poolSize; i++)
            pool.push(spawn("Debug Mode - Carousel"));

        stack.push(pool[poolPointer++]);
    }
}

// This object builds items that display an Actor in a Carousel
object "Debug Mode - Item Picker - Carousel Item Builder - Actor" is "debug-mode-carousel-item-builder"
{
    name = "";
    type = "";

    fun build(itemContainer)
    {
        return itemContainer.spawn("Debug Mode - Item Picker - Carousel Item - Actor").setName(name).setType(type);
    }

    fun setName(itemName)
    {
        name = itemName;
        return this;
    }

    fun setType(itemType)
    {
        type = itemType;
        return this;
    }
}

// This object displays an Actor in a Carousel
object "Debug Mode - Item Picker - Carousel Item - Actor" is "debug-mode-item-picker-item", "debug-mode-carousel-item", "detached", "private", "entity"
{
    carousel = null;
    name = "";
    type = "";
    actor = null;



    fun setName(itemName)
    {
        name = itemName;
        actor = Actor(name);

        //assert(actor.animation.exists);
        actor.offset = actor.hotSpot;

        this.highlighted = false;
        return this;
    }

    fun setType(itemType)
    {
        type = itemType;
        return this;
    }




    // the following methods are required for all items of an Item Picker
    fun get_type()
    {
        return type;
    }

    fun get_name()
    {
        return name;
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

    fun get_highlighted()
    {
        return actor.alpha == 1.0;
    }

    fun set_highlighted(highlighted)
    {
        actor.alpha = highlighted ? 1.0 : 0.5;
    }

    fun get_carousel()
    {
        return carousel;
    }

    fun set_carousel(carouselObject)
    {
        carousel = carouselObject;
    }
}