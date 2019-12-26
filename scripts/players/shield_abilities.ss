// -----------------------------------------------------------------------------
// File: shield_abilities.ss
// Description: gives the player the possibility of performing shield abilities
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

//
// How to create new shield abilities for your characters:
//
// Create a character-specific companion object (e.g., "Surge's Shield Abilities").
// In it, spawn & setup a "Shield Abilities" object as in the example below:
//
// player = parent;
// shieldAbilities = player.spawn("Shield Abilities").setup({
//     "<shield_name>": "<shield_ability>",
//     "<shield_name>": "<shield_ability>",
//     ...
// });
//
// where <shield_ability> is the name of an object tagged "shield ability" that
// optionally implements functions onActivate() and onDeactivate(), both taking
// a shieldAbilities object as a parameter. Object "Shield Abilities" must be a
// child of the target player, as in the example above - i.e., call player.spawn().
// See file surge.ss for a working example.
//
object "Shield Abilities" is "companion"
{
    public readonly player = parent;
    ability = { };
    enabled = true;
    timeMidAir = 0;
    currentAbility = null;

    state "main"
    {
        state = "unlocked";
    }

    state "locked"
    {
        enabled = enabled || !player.midair;
        if(mustDeactivate())
            deactivate();
    }

    state "unlocked"
    {
        enabled = enabled || !player.midair;
        timeMidAir += (player.midair ? Time.delta : -timeMidAir);
        if(timeMidAir >= 0.1 && player.input.buttonPressed("fire1") && isReady())
            activate();
        else if(mustDeactivate())
            deactivate();
    }

    state "cooldown"
    {
        if(timeout(0.016))
            state = "unlocked";
    }

    fun lock()
    {
        state = "locked";
    }

    fun unlock()
    {
        if(state != "unlocked")
            state = "cooldown";
    }

    fun getReadyToActivate() // press fire1 while midair to activate the shield attack
    {
        enabled = true; // force enable
    }

    fun get_active()
    {
        return (currentAbility != null);
    }

    fun setup(abilities)
    {
        ability.clear();
        foreach(entry in abilities)
            ability[entry.key] = spawn(entry.value);
        return this;
    }

    fun activate()
    {
        // attacks are temporarily disabled
        if(!enabled || currentAbility != null)
            return;

        // select attack depending on the shield type
        specialMove = ability[player.shield];

        // validity check
        if(specialMove == null || !specialMove.hasTag("shield ability"))
            return;

        // disable new shield attacks until
        // the player touches the ground
        enabled = false;

        // start the shield attack
        player.aggressive = true;
        player.roll();

        // perform the attack
        currentAbility = specialMove;
        if(currentAbility.hasFunction("onActivate"))
            currentAbility.onActivate(this);
    }

    fun deactivate()
    {
        if(currentAbility != null) {
            if(currentAbility.hasFunction("onDeactivate"))
                currentAbility.onDeactivate(this);
            currentAbility = null;
            player.aggressive = false;
        }
    }

    // ----------------------------

    fun isReady()
    {
        return (currentAbility == null) &&
               player.midair && (enabled || player.jumping || player.rolling || player.springing);
    }

    fun mustDeactivate()
    {
        return (currentAbility != null) &&
               (!player.midair || player.hit || player.dying || player.frozen ||
               (player.springing && player.ysp < 0));
    }
}