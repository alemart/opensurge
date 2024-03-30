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
using SurgeEngine;

object "Debug Mode - Carousel" is "debug-mode-ui-component", "iterable", "detached", "private", "entity"
{
    public readonly transform = Transform();
    itemContainers = [];
    itemHeight = 32;
    itemWidth = 32;
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
            if(transform.localPosition.x > 0 || width <= 0)
                transform.translateBy(-transform.localPosition.x, 0);
            else if(width > 0 && transform.localPosition.x < -width)
                transform.translateBy(-transform.localPosition.x - width, 0);
        }
    }

    fun add(itemBuilder)
    {
        itemContainer = spawn("Debug Mode - Carousel Item Container");
        item = itemBuilder.build(itemContainer);
        itemContainer.init(item, itemContainers.length, itemWidth, itemHeight, zindex, this);
        itemContainers.push(itemContainer);

        return this;
    }

    fun clear()
    {
        foreach(container in itemContainers)
            container.destroy();

        itemContainers.clear();
        return this;
    }

    fun get_width()
    {
        return itemWidth * itemContainers.length;
    }

    fun get_height()
    {
        return itemHeight;
    }

    fun iterator()
    {
        return spawn("Debug Mode - Carousel Iterator").setItemContainers(itemContainers);
    }

    fun setItemSize(width, height)
    {
        itemWidth = width;
        itemHeight = height;
        return this;
    }

    fun onLoad(debugMode)
    {
        tapDetector = debugMode.plugin("Debug Mode - Tap Detector");
        tapDetector.subscribe(this);

        uiSettings = debugMode.plugin("Debug Mode - UI Settings");
        zindex = uiSettings.zindex + 500;
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
            if(itemIndex >= 0 && itemIndex < itemContainers.length) {
                // get the picked item
                pickedCarouselItem = itemContainers[itemIndex].item;

                // dehighlight all items
                foreach(item in this)
                    item.highlighted = false;

                // highlight the picked item
                pickedCarouselItem.highlighted = true;

                // callback
                parent.onPickCarouselItem(pickedCarouselItem);
            }
        }
    }
}

object "Debug Mode - Carousel Item" is "debug-mode-carousel-item", "detached", "private", "entity"
{
    public carousel = null;
    public zindex = 0;
    public highlighted = false;
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

    fun init(itemReference, itemIndex, itemWidth, itemHeight, itemZindex, carousel)
    {
        item = itemReference;
        item.highlighted = false;
        item.zindex = itemZindex;
        item.carousel = carousel;

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