#include <core/Engine.h>
#include <app/App.h>
#include <utils/Logger.h>
#include <cstdlib>

int main()
{
    try
    {
        Engine engine;
        engine.Init();

        App game(engine);  //app class, registers callbacks

        engine.Run([&]()
            {
                game.Load();    //scene setup before first frame
            });

        engine.Shutdown();
    }
    catch (const std::exception& e)
    {
        Logger::Error(e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}