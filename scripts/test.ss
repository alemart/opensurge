using SurgeEngine.Actor;

object "TestEntity" is "entity", "awake"
{
    actor = Actor("SD_SURGE");
    t = Time.time;
    k = 1;

    state "main"
    {
        actor.anim = 2;
    }

    fun destructor()
    {
        Console.print("fim");
    }
}

object "SD_NEON" is "entity"
{
    actor = Actor("SD_NEON");
}

object "Application"
{
    test = spawn("TestEntity");

    state "main"
    {
    }
}