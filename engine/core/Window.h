#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <functional>
#include <string>

class Window
{
public:
    Window() = default;
    ~Window() = default;

    void Init(uint32_t width, uint32_t height, const std::string& title);
    void Shutdown();

    void PollEvents() const;
    bool ShouldClose() const;

    GLFWwindow* GetHandle() const { return m_window; }
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }

    //callbacks
    void SetKeyCallback(std::function<void(int, int)> cb) { m_keyCb = std::move(cb); }
    void SetMouseMoveCallback(std::function<void(double, double)> cb) { m_mouseCb = std::move(cb); }
    void SetMouseButtonCallback(std::function<void(int, int)> cb) { m_mouseButtonCb = std::move(cb); }

private:
    GLFWwindow* m_window = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    std::string m_title;

    std::function<void(int, int)> m_keyCb;
    std::function<void(double, double)> m_mouseCb;
    std::function<void(int, int)> m_mouseButtonCb;

    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;
    bool   m_firstMouse = true;

    static void KeyCallback(GLFWwindow*, int key, int, int action, int);
    static void CursorCallback(GLFWwindow*, double x, double y);
    static void MouseButtonCallback(GLFWwindow*, int button, int action, int);
};