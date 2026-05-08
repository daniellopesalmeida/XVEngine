#include "Window.h"
#include <stdexcept>
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

    glfwSetWindowUserPointer(m_window, this);
    glfwSetKeyCallback(m_window, KeyCallback);
    glfwSetCursorPosCallback(m_window, CursorCallback);
    glfwSetMouseButtonCallback(m_window, MouseButtonCallback);
    glfwSetFramebufferSizeCallback(m_window, FramebufferSizeCallback);

    Logger::Info("Window initialized: ", m_width, "x", m_height, " - ", m_title);
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

void Window::KeyCallback(GLFWwindow* w, int key, int, int action, int)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
    if (self->m_keyCb) self->m_keyCb(key, action);
}

void Window::CursorCallback(GLFWwindow* w, double x, double y)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
    if (self->m_firstMouse)
    {
        self->m_lastMouseX = x;
        self->m_lastMouseY = y;
        self->m_firstMouse = false;
    }
    double dx = x - self->m_lastMouseX;
    double dy = y - self->m_lastMouseY;
    self->m_lastMouseX = x;
    self->m_lastMouseY = y;
    if (self->m_mouseCb) self->m_mouseCb(dx, dy);
}

void Window::MouseButtonCallback(GLFWwindow* w, int button, int action, int)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
    if (self->m_mouseButtonCb) self->m_mouseButtonCb(button, action);
}

void Window::FramebufferSizeCallback(GLFWwindow* w, int width, int height)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
    self->m_width = static_cast<uint32_t>(width);
    self->m_height = static_cast<uint32_t>(height);
    self->m_resized = true;
    if (self->m_resizeCb) self->m_resizeCb(self->m_width, self->m_height);
}