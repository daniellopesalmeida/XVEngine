#pragma once

#include <iostream>

class Logger
{
public:
    template<typename... Args>
    static void Info(Args&&... args)
    {
        Print("\033[32m[INFO] \033[0m", std::forward<Args>(args)...); // Green
    }

    template<typename... Args>
    static void Warn(Args&&... args)
    {
        Print("\033[33m[WARN] \033[0m", std::forward<Args>(args)...); // Yellow
    }

    template<typename... Args>
    static void Error(Args&&... args)
    {
        Print("\033[31m[ERROR] \033[0m", std::forward<Args>(args)...); // Red
    }

private:
    template<typename... Args>
    static void Print(const char* prefix, Args&&... args)
    {
        std::cout << prefix;
        (std::cout << ... << args) << std::endl;
    }
};