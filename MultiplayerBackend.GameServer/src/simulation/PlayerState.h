#pragma once

#include "EntityId.h"
#include "MoveDirection.h"
#include "Vec2.h"

namespace simulation
{
    struct PlayerState
    {
        EntityId id =
            InvalidEntityId;

        Vec2 position{};

        MoveDirection movement =
            MoveDirection::None;
    };
}