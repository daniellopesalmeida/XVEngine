#include "Window.h"
#include <stdexcept>



Window::Window(int width, int height, const std::string& title)
{
    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW");

    //TODO: empty for now. need to add vulkanapi
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_Window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_Window)
        throw std::runtime_error("Failed to create GLFW window");

    
}

Window::~Window()
{
    if (m_Window)
        glfwDestroyWindow(m_Window);

    glfwTerminate();
}

void Window::PollEvents()
{
    glfwPollEvents();
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(m_Window);
}