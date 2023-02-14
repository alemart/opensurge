// -----------------------------------------------------------------------------
// File: enemy_menu.ss
// Description: enemy menu for the item picker plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

object "Debug Mode - Item Picker - Enemy Menu" is "debug-mode-plugin"
{
    fun onLoad(debugMode)
    {
        itemPicker = debugMode.plugin("Debug Mode - Item Picker");
        itemPicker.subscribe(this);

        // add to the main menu
        itemPicker
            .add(menu())
        ;

        // initialize the carousel
        /*
        carousel
            .add(menu("Skaterbug"))
            .add(entity("Lady Bugsy"))
            .add(entity("Springfling"))
            .add(entity("Wolfey"))
            .add(entity("Mosquito"))
            .add(entity("Jumping Fish"))
            .add(entity("Crococopter"))
            .add(entity("GreenMarmot"))
            .add(entity("RedMarmot"))
            .add(entity("RulerSalamander"))
            .add(entity("SwoopHarrier"))
            .add(entity("Skaterbug"))
        ;
        */
        /*
            .add(entity("Collectible"))
            .add(entity("Checkpoint"))
            .add(entity("Goal"))
            .add(entity("Goal Capsule"))
            .add(entity("Bumper"))
            .add(entity("Spikes"))
            .add(entity("Spikes Down"))
            .add(entity("Bridge"))
            .add(entity("Water Bubbles"))
            .add(entity("Powerup 1up"))
            .add(entity("Powerup Collectibles"))
            .add(entity("Powerup Lucky Bonus"))
            .add(entity("Powerup Invincibility"))
            .add(entity("Powerup Speed"))
            .add(entity("Powerup Shield"))
            .add(entity("Powerup Shield Acid"))
            .add(entity("Powerup Shield Fire"))
            .add(entity("Powerup Shield Thunder"))
            .add(entity("Powerup Shield Water"))
            .add(entity("Powerup Shield Wind"))
            .add(entity("Powerup Trap"))
            .add(entity("Spring Booster Left"))
            .add(entity("Spring Booster Right"))
            .add(entity("Spring Booster Up"))
            .add(entity("Spring Standard"))
            .add(entity("Spring Standard Down"))
            .add(entity("Spring Standard Down Left"))
            .add(entity("Spring Standard Down Right"))
            .add(entity("Spring Standard Hidden"))
            .add(entity("Spring Standard Left"))
            .add(entity("Spring Standard Right"))
            .add(entity("Spring Standard Up Left"))
            .add(entity("Spring Standard Up Right"))
            .add(entity("Spring Stronger"))
            .add(entity("Spring Stronger Down"))
            .add(entity("Spring Stronger Down Left"))
            .add(entity("Spring Stronger Down Right"))
            .add(entity("Spring Stronger Hidden"))
            .add(entity("Spring Stronger Left"))
            .add(entity("Spring Stronger Right"))
            .add(entity("Spring Stronger Up Left"))
            .add(entity("Spring Stronger Up Right"))
            .add(entity("Spring Strongest"))
            .add(entity("Spring Strongest Down"))
            .add(entity("Spring Strongest Down Left"))
            .add(entity("Spring Strongest Down Right"))
            .add(entity("Spring Strongest Hidden"))
            .add(entity("Spring Strongest Left"))
            .add(entity("Spring Strongest Right"))
            .add(entity("Spring Strongest Up Left"))
            .add(entity("Spring Strongest Up Right"))
            .add(entity("Zipline Grabber"))
            .add(entity("Zipline Left"))
            .add(entity("Zipline Right"))
            .add(entity("Power Pluggy Clockwise"))
            .add(entity("Power Pluggy Counterclockwise"))
            .add(entity("Lady Bugsy"))
            .add(entity("Springfling"))
            .add(entity("Wolfey"))
            .add(entity("Mosquito"))
            .add(entity("Jumping Fish"))
            .add(entity("Crococopter"))
            .add(entity("GreenMarmot"))
            .add(entity("RedMarmot"))
            .add(entity("RulerSalamander"))
            .add(entity("SwoopHarrier"))
            .add(entity("Skaterbug"))
        */
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
            .setName("Bumper") // sprite name
            .setType("open-enemy-menu") // item type
        ;
    }

    fun back()
    {
        return spawn("Debug Mode - Item Picker - Carousel Item Builder - Actor")
            .setName("Skaterbug") // sprite name
            .setType("close-enemy-menu") // item type
        ;
    }

    fun entity(entityName)
    {
        return spawn("Debug Mode - Item Picker - Carousel Item Builder - Actor")
            .setName(entityName) // entityName is also the name of a sprite
            .setType("entity") // item type
        ;
    }

    fun openEnemyMenu(itemPicker)
    {
        /*
        select("entity")
            .tagged(["enemy"])
            .notTagged(["private", "detached"])
        .evaluate();
        */

        itemPicker.push()
            .add(back())
            .add(entity("Skaterbug")) // friend
            .add(entity("Lady Bugsy"))
            .add(entity("Springfling"))
            .add(entity("Wolfey"))
            .add(entity("Mosquito"))
            .add(entity("Jumping Fish"))
            .add(entity("Crococopter"))
            .add(entity("GreenMarmot"))
            .add(entity("RedMarmot"))
            .add(entity("RulerSalamander"))
            .add(entity("SwoopHarrier"))
        ;
    }

    fun closeEnemyMenu(itemPicker)
    {
        itemPicker.pop();
    }

    fun onPickItem(item)
    {
        if(item.type == "open-enemy-menu")
            openEnemyMenu(item.picker);
        else if(item.type == "close-enemy-menu")
            closeEnemyMenu(item.picker);
    }
}