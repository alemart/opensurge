// -----------------------------------------------------------------------------
// File: powerups_menu.ss
// Description: powerups menu for the item picker plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

object "Debug Mode - Item Picker - Powerups Menu" is "debug-mode-plugin"
{
    select = spawn("Debug Mode - Tag Selector");

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
            .setName("Debug Mode - Item Picker - Powerups") // sprite name
            .setType("open-powerups-menu") // item type
        ;
    }

    fun back()
    {
        return spawn("Debug Mode - Item Picker - Carousel Item Builder - Actor")
            .setName("Debug Mode - Item Picker - Back") // sprite name
            .setType("close-powerups-menu") // item type
        ;
    }

    fun entity(entityName)
    {
        return spawn("Debug Mode - Item Picker - Carousel Item Builder - Actor")
            .setName(entityName) // entityName is also the name of a sprite
            .setType("entity") // item type
        ;
    }

    fun openPowerupsMenu(itemPicker)
    {
        // add the back button
        itemPicker.push()
            .add(back())
        ;

        // add the powerups
        query = select("entity")
            .tagged(["basic"])
            .notTagged(["private", "detached"])
            .matching(["Powerup"])
        ;

        names = query.evaluate();
        foreach(name in names)
            itemPicker.add(entity(name));
    }

    fun closePowerupsMenu(itemPicker)
    {
        itemPicker.pop();
    }

    fun onPickItem(item)
    {
        if(item.type == "open-powerups-menu")
            openPowerupsMenu(item.picker);
        else if(item.type == "close-powerups-menu")
            closePowerupsMenu(item.picker);
    }
}