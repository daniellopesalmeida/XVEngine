#include "Camera.h"
#include <GLFW/glfw3.h>
#include <algorithm>

void Camera::Init(float fovDeg, float aspect, float nearPlane, float farPlane)
{
    m_fov = fovDeg; m_aspect = aspect; m_near = nearPlane; m_far = farPlane;
    RecalcView();
    RecalcProjection();
}

void Camera::Update(float deltaTime)
{
    glm::vec3 right = glm::normalize(glm::cross(m_front, m_up));
    float speed = m_moveSpeed * deltaTime;

    if (m_keys[GLFW_KEY_W]) m_position += m_front * speed;
    if (m_keys[GLFW_KEY_S]) m_position -= m_front * speed;
    if (m_keys[GLFW_KEY_A]) m_position -= right * speed;
    if (m_keys[GLFW_KEY_D]) m_position += right * speed;
    if (m_keys[GLFW_KEY_E]) m_position += m_up * speed;
    if (m_keys[GLFW_KEY_Q]) m_position -= m_up * speed;

    RecalcView();
}

void Camera::OnKey(int key, int action)
{
    if (key >= 0 && key < 512)
    {
        if (action == GLFW_PRESS)   m_keys[key] = true;
        if (action == GLFW_RELEASE) m_keys[key] = false;
    }
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        m_mouseCapture = false;
}

void Camera::OnMouseButton(int button, int action)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT)
        m_mouseCapture = (action == GLFW_PRESS);
}

void Camera::OnMouseMove(double dx, double dy)
{
    if (!m_mouseCapture) return;
    m_yaw += static_cast<float>(dx) * m_sensitivity;
    m_pitch -= static_cast<float>(dy) * m_sensitivity;
    m_pitch = std::clamp(m_pitch, -89.f, 89.f);
    RecalcView();
}

void Camera::RecalcView()
{
    glm::vec3 dir;
    dir.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    dir.y = sin(glm::radians(m_pitch));
    dir.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_front = glm::normalize(dir);
    m_view = glm::lookAt(m_position, m_position + m_front, m_up);
}

void Camera::RecalcProjection()
{
    m_proj = glm::perspective(glm::radians(m_fov), m_aspect, m_near, m_far);
    m_proj[1][1] *= -1.f;  //vulkan Y-flip
}