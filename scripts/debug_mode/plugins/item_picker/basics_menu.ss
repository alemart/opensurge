// -----------------------------------------------------------------------------
// File: basics_menu.ss
// Description: basic items menu for the item picker plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

object "Debug Mode - Item Picker - Basics Menu" is "debug-mode-plugin"
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
            .setName("Debug Mode - Item Picker - Basics") // sprite name
            .setType("open-basics-menu") // item type
        ;
    }

    fun back()
    {
        return spawn("Debug Mode - Item Picker - Carousel Item Builder - Actor")
            .setName("Debug Mode - Item Picker - Back") // sprite name
            .setType("close-basics-menu") // item type
        ;
    }

    fun entity(entityName)
    {
        return spawn("Debug Mode - Item Picker - Carousel Item Builder - Actor")
            .setName(entityName) // entityName is also the name of a sprite
            .setType("entity") // item type
        ;
    }

    fun openBasicsMenu(itemPicker)
    {
        // add the back button
        itemPicker.push()
            .add(back())
        ;

        // add basic entities
        query = select("entity")
            .tagged(["basic"])
            .notTagged(["private", "detached"])
            .notMatching(["Spring", "Powerup"])
        ;

        names = query.evaluate();
        foreach(name in names)
            itemPicker.add(entity(name));

        // add springs
        query = select("entity")
            .tagged(["basic"])
            .notTagged(["private", "detached"])
            .matching(["Spring"])
        ;

        names = query.evaluate();
        foreach(name in names)
            itemPicker.add(entity(name));
    }

    fun closeBasicsMenu(itemPicker)
    {
        itemPicker.pop();
    }

    fun onPickItem(item)
    {
        if(item.type == "open-basics-menu")
            openBasicsMenu(item.picker);
        else if(item.type == "close-basics-menu")
            closeBasicsMenu(item.picker);
    }
}