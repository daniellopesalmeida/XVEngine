#pragma once
#include <cstdint>

struct ObjectHandle
{
    uint32_t id = UINT32_MAX;

    bool IsValid() const { return id != UINT32_MAX; }
    bool operator==(const ObjectHandle&) const = default;
};

constexpr ObjectHandle NullHandle = { UINT32_MAX };