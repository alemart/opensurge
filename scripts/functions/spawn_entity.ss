// -----------------------------------------------------------------------------
// File: spawn_entity.ss
// Description: a function object that spawns an entity
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;

//
// Spawn Entity is a function object that spawns an entity
// at a certain position (coordinates given in world space)
//
// Arguments:
// - entityName: string. The name of the entity to be spawned.
// - position: Vector2 object. A point given in world coordinates.
//
object "Spawn Entity"
{
    fun call(entityName, position)
    {
        Level.spawnEntity(entityName, position);
    }
}