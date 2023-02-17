// -----------------------------------------------------------------------------
// File: grid_menu.ss
// Description: grid menu for the item picker plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

object "Debug Mode - Item Picker - Grid Menu" is "debug-mode-plugin"
{
    select = spawn("Debug Mode - Tag Selector");
    grid = null;

    fun onLoad(debugMode)
    {
        grid = debugMode.plugin("Debug Mode - Grid System");

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
            .setName("Debug Mode - Item Picker - Grid") // sprite name
            .setType("open-grid-menu") // item type
        ;
    }

    fun back()
    {
        return spawn("Debug Mode - Item Picker - Carousel Item Builder - Actor")
            .setName("Debug Mode - Item Picker - Back") // sprite name
            .setType("close-grid-menu") // item type
        ;
    }

    fun button(spriteName)
    {
        return spawn("Debug Mode - Item Picker - Carousel Item Builder - Actor")
            .setName(spriteName)
            .setType("change-grid-size") // item type
        ;
    }

    fun openGridMenu(itemPicker)
    {
        itemPicker.push()
            .add(back())
            .add(button("Debug Mode - Item Picker - Grid - 16 pixels"))
            .add(button("Debug Mode - Item Picker - Grid - 8 pixels"))
            .add(button("Debug Mode - Item Picker - Grid - Off"))
        ;
    }

    fun closeGridMenu(itemPicker)
    {
        itemPicker.pop();
    }

    fun onPickItem(item)
    {
        if(item.type == "open-grid-menu") {
            openGridMenu(item.picker);
        }
        else if(item.type == "close-grid-menu") {
            closeGridMenu(item.picker);
        }
        else if(item.type == "change-grid-size") {

            if(item.name == "Debug Mode - Item Picker - Grid - 16 pixels")
                grid.size = 16;
            else if(item.name == "Debug Mode - Item Picker - Grid - 8 pixels")
                grid.size = 8;
            else if(item.name == "Debug Mode - Item Picker - Grid - Off")
                grid.size = 1;

            if(grid.size > 1)
                Console.print("Set grid size to " + grid.size + "px");
            else
                Console.print("Disabled grid");

        }
    }
}