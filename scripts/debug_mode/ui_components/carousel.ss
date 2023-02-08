// -----------------------------------------------------------------------------
// File: carousel.ss
// Description: carousel UI component (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*

This UI component lets you pick items in a Carousel.

*/

using SurgeEngine.Video.Screen;
using SurgeEngine.Transform;
using SurgeEngine.Vector2;

object "Debug Mode - Carousel" is "debug-mode-ui-component", "iterable", "detached", "private", "entity"
{
    transform = Transform();
    itemContainers = [];
    itemWidth = 48;
    itemHeight = 48;
    uiScroller = spawn("Debug Mode - UI Scroller");
    zindex = 0;

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
    }

    fun add(itemBuilder)
    {
        itemContainer = spawn("Debug Mode - Carousel Item Container");
        item = itemBuilder.build(itemContainer);
        itemContainer.init(item, itemContainers.length, itemWidth, itemHeight, zindex);
        itemContainers.push(itemContainer);

        return this;
    }

    fun iterator()
    {
        return spawn("Debug Mode - Carousel Iterator").setItemContainers(itemContainers);
    }

    fun onLoad(debugMode)
    {
        tapDetector = debugMode.plugin("Debug Mode - Tap Detector");
        tapDetector.subscribe(this);

        uiSettings = debugMode.plugin("Debug Mode - UI Settings");
        zindex = uiSettings.zindex + 1000;
    }

    fun onUnload(debugMode)
    {
        tapDetector = debugMode.plugin("Debug Mode - Tap Detector");
        tapDetector.unsubscribe(this);
    }

    fun onTapScreen(position)
    {
        // pick an item
        if(uiScroller.isInActiveArea(position)) {
            itemIndex = Math.floor((position.x - transform.localPosition.x) / itemWidth);
            if(itemIndex >= 0 && itemIndex < itemContainers.length)
                parent.onPickCarouselItem(itemContainers[itemIndex].item);
        }
    }
}

object "Debug Mode - Carousel Item" is "debug-mode-carousel-item", "detached", "private", "entity"
{
    public zindex = 0;
    public readonly naturalWidth = 1;
    public readonly naturalHeight = 1;
}

object "Debug Mode - Carousel Item Builder" is "debug-mode-carousel-item-builder"
{
    fun build(itemContainer)
    {
        return null;
        //return itemContainer.spawn("...");
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