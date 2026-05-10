#pragma once
#include <glm/glm.hpp>
#include <cstdint>

// Maximum number of lights that can be active in a single frame.
// Raise if needed — each GpuLight costs 4 × vec4 = 64 bytes,
// so 16 lights = 1 KB, well within UBO limits everywhere.
static constexpr uint32_t MAX_LIGHTS = 16;

// ── Per-light GPU record ──────────────────────────────────────────────────────
// All members are vec4 so the array satisfies std140 alignment without padding.
//
//   colorAndIntensity  — xyz = linear RGB, w = intensity scalar
//   positionAndRange   — xyz = world-space position (Point/Spot), w = range
//   directionAndType   — xyz = normalised world-space direction (Directional/Spot),
//                        w = LightType cast to float (0 Dir / 1 Point / 2 Spot)
//   coneAngles         — x = cos(innerCone), y = cos(outerCone), zw = unused
//
// The shader reads directionAndType.w as an int to branch on type.
struct GpuLight
{
    glm::vec4 colorAndIntensity;  // xyz=color,     w=intensity
    glm::vec4 positionAndRange;   // xyz=position,  w=range
    glm::vec4 directionAndType;   // xyz=direction, w=type
    glm::vec4 coneAngles;         // x=cos(inner),  y=cos(outer), zw=unused
};

// ── Per-frame uniform buffer — set 0, binding 0 ───────────────────────────────
// Updated once per frame before any draw calls.
struct FrameData
{
    glm::mat4 view;
    glm::mat4 proj;

    // xyz = world-space camera position, w = unused
    glm::vec4 cameraPos;

    // xyz = ambient color (usually white), w = ambient strength scalar
    glm::vec4 ambientColorAndStrength;

    // How many entries in lights[] are valid this frame (0..MAX_LIGHTS)
    uint32_t  lightCount;
    uint32_t  _pad[3];   // keep lights[] 16-byte aligned

    GpuLight  lights[MAX_LIGHTS];
};