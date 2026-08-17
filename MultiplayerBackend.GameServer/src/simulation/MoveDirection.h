#pragma once

#include <cstdint>

namespace simulation
{
    enum class MoveDirection : std::int8_t
    {
        Backward = -1,
        None = 0,
        Forward = 1
    };
}