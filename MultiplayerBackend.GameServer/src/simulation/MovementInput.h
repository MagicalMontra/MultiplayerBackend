#pragma once

#include "EntityId.h"
#include "MoveDirection.h"

#include <cstdint>

namespace simulation
{
    struct MovementInput
    {
        EntityId entity_id =
            InvalidEntityId;

        std::uint32_t sequence = 0;

        // Client intent only.
        MoveDirection direction =
            MoveDirection::None;
    };
}