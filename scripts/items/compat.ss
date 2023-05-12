// -----------------------------------------------------------------------------
// File: compat.ss
// Description: glue-code for retro-compatibility with built-in items (legacy)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;
using SurgeEngine.Events.EntityEvent;
using SurgeEngine.Events.FunctionEvent;

object ".compat_loopgreen" is "private", "entity"
{
    obj = spawn("Layer Green");

    state "main"
    {
    }

    fun constructor()
    {
        transform = obj.child("Transform") || obj.spawn("Transform");
        transform.translateBy(16, 16);
    }
}

object ".compat_loopyellow" is "private", "entity"
{
    obj = spawn("Layer Yellow");

    state "main"
    {
    }

    fun constructor()
    {
        transform = obj.child("Transform") || obj.spawn("Transform");
        transform.translateBy(16, 16);
    }
}

object ".compat_perspikes" is "private", "entity"
{
    spikes = spawn("Spikes");

    state "main"
    {
    }

    fun constructor()
    {
        spikes.periodic = true;
    }
}

object ".compat_perceilspikes" is "private", "entity"
{
    spikes = spawn("Spikes Down");

    state "main"
    {
    }

    fun constructor()
    {
        spikes.periodic = true;
    }
}

object ".compat_switch" is "private", "entity"
{
    obj = spawn("Switch");
    transform = Transform();

    state "main"
    {
        // get the closest entities
        position = transform.position;
        door = findClosestEntity("Door");
        teleporter = findClosestEntity("Teleporter");

        // configure the switch
        if(door != null || teleporter != null) {
            doorDistance = (door != null) ? position.distanceTo(entityPosition(door)) : Math.infinity;
            teleporterDistance = (teleporter != null) ? position.distanceTo(entityPosition(teleporter)) : Math.infinity;
            if(doorDistance < teleporterDistance) {
                obj.sticky = false;
                obj.onActivate = EntityEvent(door).willCall("open");
                obj.onDeactivate = EntityEvent(door).willCall("close");
            }
            else {
                obj.sticky = true;
                obj.onActivate = EntityEvent(teleporter).willCall("teleport");
            }
        }

        // done
        state = "done";
    }

    state "done"
    {
    }

    // you must ensure that entityName references objects tagged "entity"
    fun findClosestEntity(entityName)
    {
        closestEntity = null;
        closestDistance = Math.infinity;
        position = transform.position;

        entities = Level.findEntities(entityName);
        foreach(entity in entities) {
            distance = position.distanceTo(entityPosition(entity));
            if(distance < closestDistance) {
                closestEntity = entity;
                closestDistance = distance;
            }
        }

        return closestEntity;
    }

    // get the world position of an entity
    fun entityPosition(entity)
    {
        entityTransform = entity.child("Transform") || entity.spawn("Transform");
        return entityTransform.position;
    }
}