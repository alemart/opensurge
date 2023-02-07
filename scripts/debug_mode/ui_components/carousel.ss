// -----------------------------------------------------------------------------
// File: carousel.ss
// Description: carousel UI component (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Video.Screen;
using SurgeEngine.Input.Mouse;
using SurgeEngine.Transform;
using SurgeEngine.Vector2;

object "Debug Mode - Carousel" is "debug-mode-ui-component", "iterable", "detached", "private", "entity"
{
    debugMode = findDebugModeObject();
    transform = Transform();
    itemContainers = [];
    itemWidth = 48;
    itemHeight = 48;
    uiScroller = spawn("Debug Mode - UI Scroller");

    state "main"
    {
        // update the position and the size of the scroller
        uiScroller.setActiveArea(0, transform.position.y, Screen.width, itemHeight);

        // scroll the items
        if(Math.abs(uiScroller.dx) > 0.1) {
            transform.translateBy(uiScroller.dx, 0);

            // keep the scroll within the boundaries of the carousel
            width = itemWidth * itemContainers.length - Screen.width;
            if(transform.localPosition.x > 0)
                transform.translateBy(-transform.localPosition.x, 0);
            else if(width > 0 && transform.localPosition.x < -width)
                transform.translateBy(-transform.localPosition.x - width, 0);
        }

        // pick an item
        if(Mouse.buttonPressed("left") && uiScroller.isInActiveArea(Mouse.position)) {
            itemIndex = Math.floor((Mouse.position.x - transform.localPosition.x) / itemWidth);
            if(itemIndex >= 0 && itemIndex < itemContainers.length)
                parent.onPickCarouselItem(itemContainers[itemIndex].item);
        }
    }

    fun add(itemBuilder)
    {
        uiSettings = debugMode.plugin("Debug Mode - UI Settings");

        itemContainer = spawn("Debug Mode - Carousel Item Container");
        item = itemBuilder.build(itemContainer);
        itemContainer.init(item, itemContainers.length, itemWidth, itemHeight, uiSettings.zindex);
        itemContainers.push(itemContainer);

        return this;
    }

    fun iterator()
    {
        return spawn("Debug Mode - Carousel Iterator").setItemContainers(itemContainers);
    }

    fun findDebugModeObject()
    {
        for(obj = parent; obj != obj.parent; obj = obj.parent) {
            if(obj.__name == "Debug Mode")
                return obj;
        }

        assert(0);
        return null;
    }
}

object "Debug Mode - Carousel Item Container" is "detached", "private", "entity"
{
    transform = Transform();
    item = null; // another carousel item

    fun get_item()
    {
        return item;
    }

    fun init(itemReference, itemIndex, itemWidth, itemHeight, itemZindex)
    {
        item = itemReference;
        item.zindex = itemZindex;

        sx = itemWidth / item.naturalWidth;
        sy = itemHeight / item.naturalHeight;
        scale = Math.min(sx, sy);

        transform.localScale = Vector2(scale, scale);
        transform.localPosition = Vector2(itemWidth * itemIndex, 0);

        return this;
    }
}

object "Debug Mode - Carousel Item Builder"
{
    fun build(itemContainer)
    {
        return null;
        //return itemContainer.spawn("...");
    }
}

object "Debug Mode - Carousel Iterator" is "iterator"
{
    itemContainers = null;
    index = 0;

    fun setItemContainers(array)
    {
        itemContainers = array;
        return this;
    }

    fun hasNext()
    {
        return index < itemContainers.length;
    }

    fun next()
    {
        if(!hasNext())
            return null;

        return itemContainers[index++].item;
    }
}