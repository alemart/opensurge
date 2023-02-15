// -----------------------------------------------------------------------------
// File: specials_menu.ss
// Description: special entities menu for the item picker plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

object "Debug Mode - Item Picker - Specials Menu" is "debug-mode-plugin"
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
            .setName("Debug Mode - Item Picker - Specials") // sprite name
            .setType("open-specials-menu") // item type
        ;
    }

    fun back()
    {
        return spawn("Debug Mode - Item Picker - Carousel Item Builder - Actor")
            .setName("Debug Mode - Item Picker - Back") // sprite name
            .setType("close-specials-menu") // item type
        ;
    }

    fun entity(entityName)
    {
        return spawn("Debug Mode - Item Picker - Carousel Item Builder - Actor")
            .setName(entityName) // entityName is also the name of a sprite
            .setType("entity") // item type
        ;
    }

    fun openSpecialsMenu(itemPicker)
    {
        // add the back button
        itemPicker.push()
            .add(back())
        ;

        // add hand-picked special entities
        itemPicker
            .add(entity("Layer Green"))
            .add(entity("Layer Yellow"))
            .add(entity("Walk on Water Left"))
            .add(entity("Walk on Water Right"))
            .add(entity("Tube In"))
            .add(entity("Tube Out"))
            .add(entity("Pipe In"))
            .add(entity("Pipe Out"))
            .add(entity("Pipe Up"))
            .add(entity("Pipe Right"))
            .add(entity("Pipe Down"))
            .add(entity("Pipe Left"))
        ;

        /*
        // add the special entities
        query = select("entity")
            .tagged(["special"])
            .notTagged(["private", "detached"])
        ;

        names = query.evaluate();
        foreach(name in names)
            itemPicker.add(entity(name));
        */
    }

    fun closeSpecialsMenu(itemPicker)
    {
        itemPicker.pop();
    }

    fun onPickItem(item)
    {
        if(item.type == "open-specials-menu")
            openSpecialsMenu(item.picker);
        else if(item.type == "close-specials-menu")
            closeSpecialsMenu(item.picker);
    }
}