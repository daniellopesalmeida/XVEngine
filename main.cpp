#include "engine/core/App.h"
#include<utils/Logger.h>
#include <cstdlib>
#include <exception>

int main()
{
    try
    {
        App app;
        app.Run();
    }
    catch (const std::exception& e)
    {
        Logger::Error(e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}