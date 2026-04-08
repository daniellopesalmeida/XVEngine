#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

class Window {
public:
    Window() = default;
    ~Window() = default;

    void Init(uint32_t width, uint32_t height, const std::string& title);
    void Shutdown();

    void PollEvents() const;
    bool ShouldClose() const;

    GLFWwindow* GetHandle() const { return m_window; }
    uint32_t GetWidth()  const { return m_width; }
    uint32_t GetHeight() const { return m_height; }

private:
    GLFWwindow* m_window = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    std::string m_title;
};