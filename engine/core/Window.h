#pragma once

#include <GLFW/glfw3.h>
#include <string>


class Window
{
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    void PollEvents();
    bool ShouldClose() const;

    GLFWwindow* GetNativeWindow() const { return m_Window; }

private:
    GLFWwindow* m_Window;
};