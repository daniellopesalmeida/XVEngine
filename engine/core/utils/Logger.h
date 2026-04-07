#pragma once

#include <iostream>

class Logger
{
public:
    template<typename... Args>
    static void Info(Args&&... args)
    {
        Print("[INFO] ", std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Warn(Args&&... args)
    {
        Print("[WARN] ", std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Error(Args&&... args)
    {
        Print("[ERROR] ", std::forward<Args>(args)...);
    }

private:
    template<typename... Args>
    static void Print(const char* prefix, Args&&... args)
    {
        std::cout << prefix;
        (std::cout << ... << args) << std::endl;
    }
};