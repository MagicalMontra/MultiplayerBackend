#pragma once

#include "EntityId.h"
#include "MovementInput.h"
#include "PlayerState.h"
#include "SimulationStep.h"

#include <cstdint>
#include <variant>

namespace simulation
{
    struct SpawnPlayerEvent
    {
        PlayerState player;
    };

    struct DespawnEntityEvent
    {
        EntityId entity_id =
            InvalidEntityId;
    };

    using SimulationEventPayload =
        std::variant<
            MovementInput,
            SpawnPlayerEvent,
            DespawnEntityEvent>;

    struct SimulationEvent
    {
        // Stable ordering for events that occur at the
        // exact same GameTime.
        std::uint64_t order = 0;

        GameTime time{0};

        SimulationEventPayload payload;
    };
}