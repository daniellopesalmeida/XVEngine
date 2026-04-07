#include "engine/core/App.h"
#include <iostream>
#include<core/utils/Logger.h>

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