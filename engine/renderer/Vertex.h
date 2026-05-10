#pragma once
#include <renderer/VulkanIncludes.h>
#include <glm/glm.hpp>
#include <array>

struct Vertex
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec3 tangent;   // world-space tangent for TBN normal mapping
    // bitangent = cross(normal, tangent), computed in shader

    static vk::VertexInputBindingDescription GetBindingDescription()
    {
        vk::VertexInputBindingDescription desc{};
        desc.binding = 0;
        desc.stride = sizeof(Vertex);
        desc.inputRate = vk::VertexInputRate::eVertex;
        return desc;
    }

    static std::array<vk::VertexInputAttributeDescription, 5> GetAttributeDescriptions()
    {
        std::array<vk::VertexInputAttributeDescription, 5> attrs{};

        // location 0 — position
        attrs[0].binding = 0;
        attrs[0].location = 0;
        attrs[0].format = vk::Format::eR32G32B32Sfloat;
        attrs[0].offset = offsetof(Vertex, position);

        // location 1 — color
        attrs[1].binding = 0;
        attrs[1].location = 1;
        attrs[1].format = vk::Format::eR32G32B32Sfloat;
        attrs[1].offset = offsetof(Vertex, color);

        // location 2 — normal
        attrs[2].binding = 0;
        attrs[2].location = 2;
        attrs[2].format = vk::Format::eR32G32B32Sfloat;
        attrs[2].offset = offsetof(Vertex, normal);

        // location 3 — texCoord
        attrs[3].binding = 0;
        attrs[3].location = 3;
        attrs[3].format = vk::Format::eR32G32Sfloat;
        attrs[3].offset = offsetof(Vertex, texCoord);

        // location 4 — tangent
        attrs[4].binding = 0;
        attrs[4].location = 4;
        attrs[4].format = vk::Format::eR32G32B32Sfloat;
        attrs[4].offset = offsetof(Vertex, tangent);

        return attrs;
    }
};