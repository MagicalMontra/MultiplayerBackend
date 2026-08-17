#pragma once

#include <chrono>
#include <cstdint>

namespace simulation
{
    // Time measured from the beginning of the simulation.
    //
    // This is NOT wall-clock/calendar time.
    //
    // Example:
    //
    // 0 ns
    // 83,333,333 ns
    // 166,666,666 ns
    // ...
    using GameTime =
        std::chrono::nanoseconds;

    struct SimulationStep
    {
        // Which authoritative server step this is.
        std::uint64_t tick;

        // Simulation-time interval covered by this step.
        GameTime start_time;
        GameTime end_time;
        GameTime delta_time;

        double DeltaSeconds() const noexcept
        {
            return std::chrono::duration<double>(
                delta_time).count();
        }
    };
}