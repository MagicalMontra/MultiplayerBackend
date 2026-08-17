#include "WorldSimulation.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <utility>

namespace simulation
{
    WorldSimulation::WorldSimulation()
    {
        // Tick 0 intentionally starts with an empty world.
        //
        // Even our demo entities are created through the
        // replayable event timeline.
        state_history_.Save(
            0,
            state_);

        // -------------------------------------------------
        // Temporary demo world.
        //
        // These are events at GameTime 0, not direct
        // mutations of WorldState.
        // -------------------------------------------------

        SchedulePlayerSpawn(
            1,
            Vec2{
                .x = 0.0,
                .y = 0.0
            },
            GameTime{0});

        SubmitEvent(
            GameTime{0},
            MovementInput{
                .entity_id = 1,
                .sequence = 0,
                .direction =
                    MoveDirection::Forward
            });

        SchedulePlayerSpawn(
            2,
            Vec2{
                .x = 4.0,
                .y = 0.0
            },
            GameTime{0});

        SchedulePlayerSpawn(
            3,
            Vec2{
                .x = 11.0,
                .y = 0.0
            },
            GameTime{0});

        SchedulePlayerSpawn(
            4,
            Vec2{
                .x = -6.0,
                .y = 0.0
            },
            GameTime{0});
    }

    void WorldSimulation::Step(
        const SimulationStep& step)
    {
        assert(
            step.tick ==
            current_tick_ + 1);

        assert(
            step.start_time ==
            current_time_);

        step_history_.Save(
            step.tick,
            step);

        SimulateStep(step);

        state_history_.Save(
            step.tick,
            state_);

        current_tick_ =
            step.tick;

        current_time_ =
            step.end_time;

        PruneEventHistory();
    }

    bool WorldSimulation::SubmitMovementInput(
        const MovementInput& input,
        GameTime time)
    {
        if (input.entity_id ==
            InvalidEntityId)
        {
            return false;
        }

        if (time.count() < 0)
        {
            return false;
        }

        // At the moment we only accept movement from an
        // entity which exists in the current world.
        //
        // Later we will make validation historical so an
        // input can be validated against entity existence
        // at the input's actual GameTime.
        if (state_.FindPlayer(
                input.entity_id) == nullptr)
        {
            return false;
        }

        if (HasMovementSequence(
                input.entity_id,
                input.sequence))
        {
            return false;
        }

        return SubmitEvent(
            time,
            input);
    }

    bool WorldSimulation::SchedulePlayerSpawn(
        EntityId entity_id,
        Vec2 position,
        GameTime time)
    {
        if (entity_id ==
            InvalidEntityId)
        {
            return false;
        }

        if (time.count() < 0)
        {
            return false;
        }

        // Don't create another spawn for an entity which
        // already exists in the current world.
        if (state_.FindPlayer(
                entity_id) != nullptr)
        {
            return false;
        }

        return SubmitEvent(
            time,
            SpawnPlayerEvent{
                .player =
                    PlayerState{
                        .id = entity_id,
                        .position = position,
                        .movement =
                            MoveDirection::None
                    }
            });
    }

    bool WorldSimulation::ScheduleEntityDespawn(
        EntityId entity_id,
        GameTime time)
    {
        if (entity_id ==
            InvalidEntityId)
        {
            return false;
        }

        if (time.count() < 0)
        {
            return false;
        }

        return SubmitEvent(
            time,
            DespawnEntityEvent{
                .entity_id =
                    entity_id
            });
    }

    const WorldState&
    WorldSimulation::State() const noexcept
    {
        return state_;
    }

    std::vector<EntityId>
    WorldSimulation::QueryNearbyEntities(
        EntityId observer_id,
        int cell_radius) const
    {
        const PlayerState* observer =
            state_.FindPlayer(
                observer_id);

        if (observer == nullptr)
        {
            return {};
        }

        auto entities =
            spatial_grid_.QueryNeighborhood(
                observer->position,
                cell_radius);

        entities.erase(
            std::remove(
                entities.begin(),
                entities.end(),
                observer_id),
            entities.end());

        return entities;
    }

    std::uint64_t
    WorldSimulation::CurrentTick() const noexcept
    {
        return current_tick_;
    }

    GameTime
    WorldSimulation::CurrentTime() const noexcept
    {
        return current_time_;
    }

    void WorldSimulation::SimulateStep(
        const SimulationStep& step)
    {
        GameTime cursor =
            step.start_time;

        auto event_it =
            std::lower_bound(
                events_.begin(),
                events_.end(),
                step.start_time,
                [](
                    const SimulationEvent& event,
                    GameTime time)
                {
                    return event.time <
                        time;
                });

        // -------------------------------------------------
        // Step interval semantics:
        //
        // [start_time, end_time)
        //
        // Event exactly at start:
        //     belongs to this step.
        //
        // Event exactly at end:
        //     belongs to next step.
        // -------------------------------------------------

        while (
            event_it != events_.end() &&
            event_it->time <
                step.end_time)
        {
            // Advance the world up to the exact event
            // time using the state that was active before
            // the event.
            SimulateInterval(
                cursor,
                event_it->time);

            ApplyEvent(
                *event_it);

            cursor =
                event_it->time;

            ++event_it;
        }

        SimulateInterval(
            cursor,
            step.end_time);
    }

    void WorldSimulation::SimulateInterval(
        GameTime start_time,
        GameTime end_time)
    {
        if (end_time <=
            start_time)
        {
            return;
        }

        const double delta_seconds =
            std::chrono::duration<double>(
                end_time -
                start_time)
                .count();

        for (auto& player :
             state_.players)
        {
            const int direction =
                static_cast<int>(
                    player.movement);

            if (direction == 0)
            {
                continue;
            }

            player.position.x +=
                static_cast<double>(
                    direction) *
                MoveSpeed *
                delta_seconds;

            spatial_grid_.Update(
                player.id,
                player.position);
        }
    }

    void WorldSimulation::ApplyEvent(
        const SimulationEvent& event)
    {
        // std::get_if<T>() returns:
        //
        // T*       when the variant currently contains T
        // nullptr  otherwise.

        if (const auto* movement =
                std::get_if<MovementInput>(
                    &event.payload))
        {
            PlayerState* player =
                state_.FindPlayer(
                    movement->entity_id);

            // A valid simulation timeline should never
            // contain movement for a nonexistent entity.
            assert(player != nullptr);

            if (player == nullptr)
            {
                return;
            }

            player->movement =
                movement->direction;

            return;
        }

        if (const auto* spawn =
                std::get_if<SpawnPlayerEvent>(
                    &event.payload))
        {
            const bool spawned =
                state_.SpawnPlayer(
                    spawn->player);

            assert(spawned);

            if (!spawned)
            {
                return;
            }

            spatial_grid_.Insert(
                spawn->player.id,
                spawn->player.position);

            return;
        }

        if (const auto* despawn =
                std::get_if<DespawnEntityEvent>(
                    &event.payload))
        {
            const bool despawned =
                state_.DespawnEntity(
                    despawn->entity_id);

            assert(despawned);

            if (!despawned)
            {
                return;
            }

            spatial_grid_.Remove(
                despawn->entity_id);

            return;
        }

        // Every variant alternative should have been
        // handled above.
        assert(false);
    }

    bool WorldSimulation::SubmitEvent(
        GameTime time,
        SimulationEventPayload payload)
    {
        if (time.count() < 0)
        {
            return false;
        }

        std::optional<std::uint64_t>
            restore_tick;

        if (time < current_time_)
        {
            restore_tick =
                FindRestoreTick(
                    time);

            if (!restore_tick.has_value())
            {
                return false;
            }
        }

        const std::uint64_t order =
            next_event_order_++;

        InsertEvent(
            SimulationEvent{
                .order = order,
                .time = time,
                .payload =
                    std::move(payload)
            });

        if (!restore_tick.has_value())
        {
            return true;
        }

        if (RollbackFrom(
                *restore_tick))
        {
            return true;
        }

        // Rollback should only fail if our retained
        // histories are inconsistent, but don't leave a
        // failed event inside the canonical timeline.
        RemoveEvent(order);

        return false;
    }

    std::optional<std::uint64_t>
    WorldSimulation::FindRestoreTick(
        GameTime event_time) const
    {
        for (std::uint64_t tick =
                 current_tick_;
             tick > 0;
             --tick)
        {
            const SimulationStep* step =
                step_history_.Find(
                    tick);

            if (step == nullptr)
            {
                break;
            }

            if (event_time >=
                    step->start_time &&
                event_time <
                    step->end_time)
            {
                const std::uint64_t
                    restore_tick =
                        tick - 1;

                if (state_history_.Find(
                        restore_tick) ==
                    nullptr)
                {
                    return std::nullopt;
                }

                return restore_tick;
            }
        }

        return std::nullopt;
    }

    bool WorldSimulation::RollbackFrom(
        std::uint64_t restore_tick)
    {
        const std::uint64_t present_tick =
            current_tick_;

        if (!state_history_.Restore(
                restore_tick,
                state_))
        {
            return false;
        }

        RebuildSpatialIndex();

        for (std::uint64_t tick =
                 restore_tick + 1;
             tick <= present_tick;
             ++tick)
        {
            const SimulationStep* step =
                step_history_.Find(
                    tick);

            if (step == nullptr)
            {
                return false;
            }

            SimulateStep(
                *step);

            // Replace the old timeline snapshot with
            // corrected authoritative history.
            state_history_.Save(
                tick,
                state_);
        }

        return true;
    }

    bool WorldSimulation::HasMovementSequence(
        EntityId entity_id,
        std::uint32_t sequence) const
    {
        return std::any_of(
            events_.begin(),
            events_.end(),
            [entity_id, sequence](
                const SimulationEvent& event)
            {
                const auto* movement =
                    std::get_if<
                        MovementInput>(
                            &event.payload);

                if (movement == nullptr)
                {
                    return false;
                }

                return
                    movement->entity_id ==
                        entity_id &&
                    movement->sequence ==
                        sequence;
            });
    }

    void WorldSimulation::InsertEvent(
        SimulationEvent event)
    {
        const auto insert_position =
            std::lower_bound(
                events_.begin(),
                events_.end(),
                event,
                [](
                    const SimulationEvent& left,
                    const SimulationEvent& right)
                {
                    if (left.time !=
                        right.time)
                    {
                        return
                            left.time <
                            right.time;
                    }

                    return
                        left.order <
                        right.order;
                });

        events_.insert(
            insert_position,
            std::move(event));
    }

    void WorldSimulation::RemoveEvent(
        std::uint64_t order)
    {
        const auto it =
            std::find_if(
                events_.begin(),
                events_.end(),
                [order](
                    const SimulationEvent& event)
                {
                    return
                        event.order ==
                        order;
                });

        if (it != events_.end())
        {
            events_.erase(it);
        }
    }

    void WorldSimulation::PruneEventHistory()
    {
        if (current_tick_ <
            HistorySize)
        {
            return;
        }

        const std::uint64_t
            oldest_restore_tick =
                current_tick_ -
                HistorySize +
                1;

        const SimulationStep*
            oldest_restore_step =
                step_history_.Find(
                    oldest_restore_tick);

        if (oldest_restore_step ==
            nullptr)
        {
            return;
        }

        const GameTime oldest_replay_time =
            oldest_restore_step->
                end_time;

        // events_ is sorted chronologically.
        const auto first_retained =
            std::lower_bound(
                events_.begin(),
                events_.end(),
                oldest_replay_time,
                [](
                    const SimulationEvent& event,
                    GameTime time)
                {
                    return
                        event.time <
                        time;
                });

        events_.erase(
            events_.begin(),
            first_retained);
    }

    void WorldSimulation::RebuildSpatialIndex()
    {
        spatial_grid_.Clear();

        for (const auto& player :
             state_.players)
        {
            spatial_grid_.Insert(
                player.id,
                player.position);
        }
    }
}