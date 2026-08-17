#pragma once

#include "MovementInput.h"
#include "SimulationEvent.h"
#include "SimulationStep.h"
#include "SpatialGrid.h"
#include "StateHistory.h"
#include "WorldState.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace simulation
{
    class WorldSimulation
    {
    public:
        WorldSimulation();

        void Step(
            const SimulationStep& step);

        bool SubmitMovementInput(
            const MovementInput& input,
            GameTime time);

        bool SchedulePlayerSpawn(
            EntityId entity_id,
            Vec2 position,
            GameTime time);

        bool ScheduleEntityDespawn(
            EntityId entity_id,
            GameTime time);

        const WorldState&
        State() const noexcept;

        std::vector<EntityId>
        QueryNearbyEntities(
            EntityId observer_id,
            int cell_radius) const;

        std::uint64_t
        CurrentTick() const noexcept;

        GameTime
        CurrentTime() const noexcept;

    private:
        static constexpr std::size_t
        HistorySize = 64;

        static constexpr double
        MoveSpeed = 1.0;

        static constexpr double
        SpatialCellSize = 5.0;

        void SimulateStep(
            const SimulationStep& step);

        void SimulateInterval(
            GameTime start_time,
            GameTime end_time);

        void ApplyEvent(
            const SimulationEvent& event);

        bool SubmitEvent(
            GameTime time,
            SimulationEventPayload payload);

        std::optional<std::uint64_t>
        FindRestoreTick(
            GameTime event_time) const;

        bool RollbackFrom(
            std::uint64_t restore_tick);

        bool HasMovementSequence(
            EntityId entity_id,
            std::uint32_t sequence) const;

        void InsertEvent(
            SimulationEvent event);

        void RemoveEvent(
            std::uint64_t order);

        void PruneEventHistory();

        void RebuildSpatialIndex();

        WorldState state_;

        StateHistory<
            WorldState,
            HistorySize> state_history_;

        StateHistory<
            SimulationStep,
            HistorySize> step_history_;

        std::vector<SimulationEvent>
            events_;

        SpatialGrid spatial_grid_{
            SpatialCellSize
        };

        std::uint64_t current_tick_ = 0;

        GameTime current_time_{0};

        std::uint64_t next_event_order_ = 1;
    };
}