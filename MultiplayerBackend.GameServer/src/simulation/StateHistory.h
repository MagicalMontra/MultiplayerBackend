#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace simulation
{
    template<
        typename T,
        std::size_t Capacity>
    class StateHistory
    {
        static_assert(
            Capacity > 0,
            "StateHistory capacity must be greater than zero");

    public:
        void Save(
            std::uint64_t tick,
            const T& state)
        {
            const std::size_t index =
                static_cast<std::size_t>(
                    tick % Capacity);

            entries_[index] =
                Entry{
                .tick = tick,
                .state = state
            };
        }

        const T* Find(
            std::uint64_t tick) const
        {
            const std::size_t index =
                static_cast<std::size_t>(
                    tick % Capacity);

            const auto& entry =
                entries_[index];

            if (!entry.has_value())
            {
                return nullptr;
            }

            if (entry->tick != tick)
            {
                return nullptr;
            }

            return &entry->state;
        }

        bool Restore(
            std::uint64_t tick,
            T& state) const
        {
            const T* historical_state =
                Find(tick);

            if (historical_state == nullptr)
            {
                return false;
            }

            state = *historical_state;

            return true;
        }

        static constexpr std::size_t
        GetCapacity() noexcept
        {
            return Capacity;
        }

    private:
        struct Entry
        {
            std::uint64_t tick;
            T state;
        };

        std::array<
            std::optional<Entry>,
            Capacity> entries_{};
    };
}