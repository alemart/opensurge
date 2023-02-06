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
using SurgeEngine.Actor;
using SurgeEngine.Level;

object "Debug Mode - Carousel" is "debug-mode-ui-component", "detached", "private", "entity"
{
    debugMode = Level.child("Debug Mode");
    transform = Transform();
    items = [];
    itemWidth = 32;
    itemHeight = 32;
    uiScroller = spawn("Debug Mode - UI Scroller");

    state "main"
    {
        // update the position and the size of the scroller
        uiScroller.setActiveArea(0, transform.position.y, Screen.width, itemHeight);

        // scroll the items
        if(Math.abs(uiScroller.dx) > 0.1) {
            transform.translateBy(uiScroller.dx, 0);

            // keep the scroll within the boundaries of the carousel
            width = itemWidth * items.length - Screen.width;
            if(transform.localPosition.x > 0)
                transform.translateBy(-transform.localPosition.x, 0);
            else if(width > 0 && transform.localPosition.x < -width)
                transform.translateBy(-transform.localPosition.x - width, 0);
        }

        // pick an item
        if(Mouse.buttonPressed("left") && uiScroller.isInActiveArea(Mouse.position)) {
            itemIndex = Math.floor((Mouse.position.x - transform.localPosition.x) / itemWidth);
            if(itemIndex >= 0 && itemIndex < items.length)
                parent.onCarouselItemPick(items[itemIndex].name);
        }
    }

    fun addItem(entityName)
    {
        uiSettings = debugMode.plugin("Debug Mode - UI Settings");
        zindex = uiSettings.zindex;

        item = spawn("Debug Mode - Carousel Item").init(entityName, items.length, itemWidth, itemHeight, zindex);
        items.push(item);

        return this;
    }

    fun setItemSize(width, height)
    {
        itemWidth = Math.max(8, width);
        itemHeight = Math.max(8, height);
        return this;
    }
}

// TODO: make this more generic
object "Debug Mode - Carousel Item" is "detached", "private", "entity"
{
    transform = Transform();
    actor = null;
    name = "";

    fun init(entityName, index, itemWidth, itemHeight, zindex)
    {
        name = entityName;
        actor = Actor(entityName);
        assert(actor.animation.exists);

        actor.offset = actor.hotSpot;
        actor.zindex = zindex;

        sx = itemWidth / actor.width;
        sy = itemHeight / actor.height;
        scale = Math.min(sx, sy);

        transform.localScale = Vector2(scale, scale);
        transform.localPosition = Vector2(itemWidth * index, 0);

        return this;
    }

    fun get_name()
    {
        return name;
    }
}