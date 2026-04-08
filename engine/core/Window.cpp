#include "Window.h"
#include <stdexcept>
#include <iostream>
#include <utils/Logger.h>

void Window::Init(uint32_t width, uint32_t height, const std::string& title)
{
    m_width = width;
    m_height = height;
    m_title = title;

    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
    if (!m_window)
        throw std::runtime_error("Failed to create GLFW window");

    Logger::Info("Window initialized: ",m_width, "x", m_height," - ", m_title);
}

void Window::Shutdown()
{
    if (m_window)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
    Logger::Info("Window shutdown");
}

bool Window::ShouldClose() const { return glfwWindowShouldClose(m_window); }
void Window::PollEvents()  const { glfwPollEvents(); }