// -----------------------------------------------------------------------------
// File: enemies_menu.ss
// Description: enemies menu for the item picker plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

object "Debug Mode - Item Picker - Enemies Menu" is "debug-mode-plugin"
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
            .setName("Lady Bugsy") // sprite name
            .setType("open-enemies-menu") // item type
        ;
    }

    fun back()
    {
        return spawn("Debug Mode - Item Picker - Carousel Item Builder - Actor")
            .setName("Skaterbug") // sprite name
            .setType("close-enemies-menu") // item type
        ;
    }

    fun entity(entityName)
    {
        return spawn("Debug Mode - Item Picker - Carousel Item Builder - Actor")
            .setName(entityName) // entityName is also the name of a sprite
            .setType("entity") // item type
        ;
    }

    fun openEnemiesMenu(itemPicker)
    {
        // add the back button
        itemPicker.push()
            .add(back())
        ;

        // add the enemies
        query = select("entity")
            .tagged(["enemy"])
            .notTagged(["private", "detached", "boss"])
        ;

        names = query.evaluate();
        foreach(name in names)
            itemPicker.add(entity(name));
    }

    fun closeEnemiesMenu(itemPicker)
    {
        itemPicker.pop();
    }

    fun onPickItem(item)
    {
        if(item.type == "open-enemies-menu")
            openEnemiesMenu(item.picker);
        else if(item.type == "close-enemies-menu")
            closeEnemiesMenu(item.picker);
    }
}