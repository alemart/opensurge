// -----------------------------------------------------------------------------
// File: tag_selector.ss
// Description: utility to select tagged objects (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*

This utility lets you query the SurgeScript runtime for objects tagged / named
in certain ways. Example:

object "Debug Mode - Testing The Tag Selector" is "debug-mode-plugin"
{
    select = spawn("Debug Mode - Tag Selector");

    fun onLoad(debugMode)
    {
        // find all (the names of the) objects tagged "entity"
        // which are neither "private" nor "detached"
        query = select("entity").notTagged([ "private", "detached" ]);
        names = query.evaluate();
        foreach(name in names)
            Console.print(name);
        Console.print("> Found " + names.length + " objects");

        // find all (the names of the) objects tagged "entity" and "awake"
        // which are neither "private" nor "detached"
        query = select("entity").tagged([ "awake" ]).notTagged([ "private", "detached" ]);
        names = query.evaluate();
        foreach(name in names)
            Console.print(name);
        Console.print("> Found " + names.length + " objects");

        // find all (the names of the) objects tagged "entity" which match "Powerup"
        query = select("entity").matching([ "Powerup" ]);
        names = query.evaluate();
        foreach(name in names)
            Console.print(name);
        Console.print("> Found " + names.length + " objects");

        // find all (the names of the) objects tagged "entity" which are
        // neither "private" nor "detached" and which do not match "Powerup"
        query = select("entity").notTagged([ "private", "detached" ]).notMatching([ "Powerup" ]);
        names = query.evaluate();
        foreach(name in names)
            Console.print(name);
        Console.print("> Found " + names.length + " objects");
    }
}

*/

object "Debug Mode - Tag Selector" is "debug-mode-utility"
{
    fun call(tagName)
    {
        return spawn("Debug Mode - Tag Selector - Query").tagged([ tagName ]);
    }
}

object "Debug Mode - Tag Selector - Query"
{
    wantedTags = [];
    unwantedTags = [];
    matchingSubstrings = [];
    notMatchingSubstrings = [];

    fun tagged(tags)
    {
        foreach(tag in tags)
            wantedTags.push(tag);

        return this;
    }

    fun notTagged(tags)
    {
        foreach(tag in tags)
            unwantedTags.push(tag);

        return this;
    }

    fun matching(names)
    {
        foreach(name in names)
            matchingSubstrings.push(name);

        return this;
    }

    fun notMatching(names)
    {
        foreach(name in names)
            notMatchingSubstrings.push(name);

        return this;
    }

    fun evaluate()
    {
        // minimize the search space
        objects = [];

        for(i = 0; i < wantedTags.length; i++) {
            wantedTag = wantedTags[i];
            objects.push(System.tags.select(wantedTag));
        }

        minIndex = 0;
        for(i = 1; i < wantedTags.length; i++) {
            if(wantedTags[i].length < wantedTags[minIndex].length)
                minIndex = i;
        }

        tmp = wantedTags[minIndex];
        wantedTags[minIndex] = wantedTags[0];
        wantedTags[0] = tmp;

        tmp = objects[minIndex];
        objects[minIndex] = objects[0];
        objects[0] = tmp;

        // filter the candidates according to their tags
        result = [];
        candidates = objects[0];
        foreach(candidate in candidates) {

            // find the tags of the candidate
            tags = System.tags.tagsOf(candidate);


            // the candidate must include all wanted tags
            wanted = true;

            for(i = 1; i < wantedTags.length && wanted; i++)
                wanted = wanted && (tags.indexOf(wantedTags[i]) >= 0);

            if(!wanted)
                continue;


            // the candidate must not include any unwanted tag
            unwanted = false;

            for(i = 0; i < unwantedTags.length && !unwanted; i++)
                unwanted = unwanted || (tags.indexOf(unwantedTags[i]) >= 0);

            if(unwanted)
                continue;


            // the name of the candidate must match all wanted substrings
            match = true;

            for(i = 0; i < matchingSubstrings.length && match; i++)
                match = match && (candidate.indexOf(matchingSubstrings[i]) >= 0);

            if(!match)
                continue;


            // the name of the candidate must not match any unwanted substrings
            notMatch = false;

            for(i = 0; i < notMatchingSubstrings.length && !notMatch; i++)
                notMatch = notMatch || (candidate.indexOf(notMatchingSubstrings[i]) >= 0);

            if(notMatch)
                continue;


            // store suitable candidates
            result.push(candidate);

        }

        // sort the results for consistency?
        // at the time of this writing, the internal implementation of
        // System.tags.select() already returns sorted results due to the way
        // things are stored, but I don't guarantee this in the specification.
        result.sort(null);

        // done!
        return result;
    }
}