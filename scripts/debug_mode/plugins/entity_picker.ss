// -----------------------------------------------------------------------------
// File: entity_picker.ss
// Description: entity picker plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Actor;

object "Debug Mode - Entity Picker" is "debug-mode-plugin", "detached", "private", "entity"
{
    debugMode = parent;
    transform = Transform();
    carousel = spawn("Debug Mode - Carousel");

    state "main"
    {
        // initialize the carousel
        carousel
            .add(entity("Collectible"))
            .add(entity("Checkpoint"))
            .add(entity("Goal"))
            .add(entity("Goal Capsule"))
            .add(entity("Bumper"))
            .add(entity("Layer Green"))
            .add(entity("Layer Yellow"))
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
            .add(entity("Spikes"))
            .add(entity("Spikes Down"))
            .add(entity("Spring Booster Left"))
            .add(entity("Spring Booster Right"))
            .add(entity("Water Bubbles"))
            .add(entity("Tube In"))
            .add(entity("Tube Out"))
        ;

        // set the position
        transform.position = Vector2.zero;

        // done!
        state = "enabled";
    }

    state "enabled"
    {
        // do nothing
    }

    fun entity(entityName)
    {
        return spawn("Debug Mode - Entity Picker - Carousel Item Builder").setEntityName(entityName);
    }

    fun onCarouselItemPick(pickedItem)
    {
        foreach(item in carousel) {
            item.highlighted = false;
        }

        pickedItem.highlighted = true;
    }
}

object "Debug Mode - Entity Picker - Carousel Item Builder"
{
    entityName = "";

    fun build(itemContainer)
    {
        return itemContainer.spawn("Debug Mode - Entity Picker - Carousel Item").setEntityName(entityName);
    }

    fun setEntityName(name)
    {
        entityName = name;
        return this;
    }
}

object "Debug Mode - Entity Picker - Carousel Item" is "detached", "private", "entity"
{
    name = "";
    actor = null;



    fun get_name()
    {
        return name;
    }

    fun get_highlighted()
    {
        return actor.alpha == 1.0;
    }

    fun set_highlighted(highlighted)
    {
        actor.alpha = highlighted ? 1.0 : 0.5;
    }

    fun setEntityName(entityName)
    {
        name = entityName;
        actor = Actor(name);

        assert(actor.animation.exists);
        actor.offset = actor.hotSpot;

        this.highlighted = false;
        return this;
    }



    // the following methods are required for all Carousel items
    // the Carousel component will use these to adjust the UI
    fun get_naturalWidth()
    {
        return actor.width;
    }

    fun get_naturalHeight()
    {
        return actor.height;
    }

    fun get_zindex()
    {
        return actor.zindex;
    }

    fun set_zindex(zindex)
    {
        actor.zindex = zindex;
    }
}