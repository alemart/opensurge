// -----------------------------------------------------------------------------
// File: gimmicks_menu.ss
// Description: gimmicks menu for the item picker plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

object "Debug Mode - Item Picker - Gimmicks Menu" is "debug-mode-plugin"
{
    fun onLoad(debugMode)
    {
        itemPicker = debugMode.plugin("Debug Mode - Item Picker");
        itemPicker.subscribe(this);

        // add to the main menu
        itemPicker
            .add(menu())
        ;
    }

    fun onUnload(debugMode)
    {
        itemPicker = debugMode.plugin("Debug Mode - Item Picker");
        itemPicker.unsubscribe(this);
    }

    fun menu()
    {
        return spawn("Debug Mode - Item Picker - Carousel Item Builder - Actor")
            .setName("Debug Mode - Item Picker - Gimmicks") // sprite name
            .setType("open-gimmicks-menu") // item type
        ;
    }

    fun back()
    {
        return spawn("Debug Mode - Item Picker - Carousel Item Builder - Actor")
            .setName("Debug Mode - Item Picker - Back") // sprite name
            .setType("close-gimmicks-menu") // item type
        ;
    }

    fun entity(entityName)
    {
        return spawn("Debug Mode - Item Picker - Carousel Item Builder - Actor")
            .setName(entityName) // entityName is also the name of a sprite
            .setType("entity") // item type
        ;
    }

    fun openGimmicksMenu(itemPicker)
    {
        itemPicker.push()
            .add(back())

            // only a few hand-picked gimmicks are available at this time
            .add(entity("Skaterbug"))
            .add(entity("Bridge"))
            .add(entity("Power Pluggy Clockwise"))
            .add(entity("Power Pluggy Counterclockwise"))
            .add(entity("Zipline Grabber"))
            .add(entity("Zipline Right"))
            .add(entity("Zipline Left"))
        ;
    }

    fun closeGimmicksMenu(itemPicker)
    {
        itemPicker.pop();
    }

    fun onPickItem(item)
    {
        if(item.type == "open-gimmicks-menu")
            openGimmicksMenu(item.picker);
        else if(item.type == "close-gimmicks-menu")
            closeGimmicksMenu(item.picker);
    }
}