#include "InterestTracker.h"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace replication
{
    InterestDelta InterestTracker::Update(
        std::vector<
            simulation::EntityId>
            desired_interest)
    {
        // Don't depend on the caller providing sorted
        // or duplicate-free data.
        std::sort(
            desired_interest.begin(),
            desired_interest.end());

        desired_interest.erase(
            std::unique(
                desired_interest.begin(),
                desired_interest.end()),
            desired_interest.end());

        InterestDelta delta;

        std::size_t current_index = 0;
        std::size_t desired_index = 0;

        // -------------------------------------------------
        // Both vectors are sorted.
        //
        // Walk them together in one pass:
        //
        // current < desired
        //     entity disappeared from interest
        //
        // desired < current
        //     new entity entered interest
        //
        // equal
        //     entity stayed relevant
        // -------------------------------------------------

        while (
            current_index <
                current_.size() &&
            desired_index <
                desired_interest.size())
        {
            const simulation::EntityId
                current_entity =
                    current_[
                        current_index];

            const simulation::EntityId
                desired_entity =
                    desired_interest[
                        desired_index];

            if (current_entity <
                desired_entity)
            {
                delta.left.push_back(
                    current_entity);

                ++current_index;

                continue;
            }

            if (desired_entity <
                current_entity)
            {
                delta.entered.push_back(
                    desired_entity);

                ++desired_index;

                continue;
            }

            // Same EntityId exists in both sets.
            delta.stayed.push_back(
                current_entity);

            ++current_index;
            ++desired_index;
        }

        // Anything remaining in current_ is no longer
        // visible.
        while (
            current_index <
            current_.size())
        {
            delta.left.push_back(
                current_[
                    current_index]);

            ++current_index;
        }

        // Anything remaining in desired_interest is new.
        while (
            desired_index <
            desired_interest.size())
        {
            delta.entered.push_back(
                desired_interest[
                    desired_index]);

            ++desired_index;
        }

        // desired_interest becomes the new persistent
        // client-visible interest state.
        current_ =
            std::move(
                desired_interest);

        return delta;
    }

    void InterestTracker::Clear()
    {
        current_.clear();
    }

    const std::vector<
        simulation::EntityId>&
    InterestTracker::Current() const noexcept
    {
        return current_;
    }
}