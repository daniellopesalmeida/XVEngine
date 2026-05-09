#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/fwd.hpp>

class Camera
{
public:
    void Init(float fovDeg, float aspect, float nearPlane, float farPlane);
    void Update(float deltaTime);
    void SetAspect(float aspect) { m_aspect = aspect; RecalcProjection(); }

    glm::mat4 GetViewProj() const { return m_proj * m_view; }
    glm::mat4 GetView() const { return m_view; }
    glm::mat4 GetProj() const { return m_proj; }
    glm::vec3 GetPosition()  const { return m_position; }

    void OnKey(int key, int action);
    void OnMouseMove(double dx, double dy);
    void OnMouseButton(int button, int action);

private:
    glm::vec3 m_position = { 0.f, 0.f,  2.f };
    glm::vec3 m_front = { 0.f, 0.f, -1.f };
    glm::vec3 m_up = { 0.f, 1.f,  0.f };

    float m_yaw = -90.f;
    float m_pitch = 0.f;

    float m_fov = 60.f;
    float m_aspect = 1.f;
    float m_near = 0.01f;
    float m_far = 1000.f;

    bool  m_keys[512] = {};
    bool  m_mouseCapture = false;
    float m_moveSpeed = 20.f;
    float m_sensitivity = 0.1f;

    glm::mat4 m_view = glm::mat4(1.f);
    glm::mat4 m_proj = glm::mat4(1.f);

    void RecalcView();
    void RecalcProjection();
};