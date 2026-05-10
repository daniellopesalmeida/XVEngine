#pragma once
#include <glm/glm.hpp>
#include <cstdint>

// ── Light types ───────────────────────────────────────────────────────────────
enum class LightType : uint32_t
{
    Directional = 0,
    Point = 1,
    Spot = 2,
};

// ── CPU-side light description ────────────────────────────────────────────────
// Set whichever fields are relevant to the chosen type:
//   Directional — direction, color, intensity
//   Point       — position, color, intensity, range
//   Spot        — position, direction, color, intensity, range, innerCone, outerCone
struct Light
{
    LightType type = LightType::Directional;

    glm::vec3 color = { 1.f, 1.f, 1.f };
    float intensity = 1.f;

    glm::vec3 position = { 0.f, 0.f, 0.f };   // Point + Spot
    glm::vec3 direction = { 0.f,-1.f, 0.f };   // Directional + Spot (need not be normalised)

    float range = 50.f;    // Point + Spot — distance at which attenuation reaches zero
    float innerCone = 15.f;    // Spot — full-intensity cone angle in degrees
    float outerCone = 30.f;    // Spot — zero-intensity cone angle in degrees
};

// ── Handle for safe external references ──────────────────────────────────────
struct LightHandle
{
    uint32_t id = UINT32_MAX;

    bool IsValid() const { return id != UINT32_MAX; }
    bool operator==(const LightHandle&) const = default;
};

constexpr LightHandle NullLightHandle = { UINT32_MAX };