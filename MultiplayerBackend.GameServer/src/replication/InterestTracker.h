#pragma once

#include "../simulation/EntityId.h"

#include <vector>

namespace replication
{
    struct InterestDelta
    {
        std::vector<simulation::EntityId>
            entered;

        std::vector<simulation::EntityId>
            stayed;

        std::vector<simulation::EntityId>
            left;
    };

    class InterestTracker
    {
    public:
        InterestDelta Update(
            std::vector<
                simulation::EntityId>
                desired_interest);

        void Clear();

        const std::vector<
            simulation::EntityId>&
        Current() const noexcept;

    private:
        std::vector<
            simulation::EntityId>
            current_;
    };
}