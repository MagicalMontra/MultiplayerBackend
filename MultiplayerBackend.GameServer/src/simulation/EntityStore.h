#pragma once

#include "EntityId.h"

#include <cstddef>
#include <unordered_map>
#include <utility>
#include <vector>

namespace simulation
{
    template<typename T>
    class EntityStore
    {
    public:
        using Iterator =
            typename std::vector<T>::iterator;

        using ConstIterator =
            typename std::vector<T>::const_iterator;

        bool Insert(
            const T& entity)
        {
            const EntityId id =
                entity.id;

            if (id == InvalidEntityId)
            {
                return false;
            }

            if (index_by_id_.contains(id))
            {
                return false;
            }

            const std::size_t index =
                entities_.size();

            entities_.push_back(
                entity);

            index_by_id_.emplace(
                id,
                index);

            return true;
        }

        bool Insert(
            T&& entity)
        {
            // Capture the ID before moving the object.
            const EntityId id =
                entity.id;

            if (id == InvalidEntityId)
            {
                return false;
            }

            if (index_by_id_.contains(id))
            {
                return false;
            }

            const std::size_t index =
                entities_.size();

            entities_.push_back(
                std::move(entity));

            index_by_id_.emplace(
                id,
                index);

            return true;
        }

        bool Remove(
            EntityId id)
        {
            const auto index_it =
                index_by_id_.find(id);

            if (index_it ==
                index_by_id_.end())
            {
                return false;
            }

            const std::size_t remove_index =
                index_it->second;

            const std::size_t last_index =
                entities_.size() - 1;

            // -------------------------------------------------
            // Swap-and-pop style removal.
            //
            // We do not care about preserving dense storage
            // order. EntityId is identity, vector index is not.
            // -------------------------------------------------

            if (remove_index != last_index)
            {
                entities_[remove_index] =
                    std::move(
                        entities_[last_index]);

                // The entity moved from the final slot into
                // remove_index, so repair its lookup entry.
                index_by_id_[
                    entities_[remove_index].id
                ] = remove_index;
            }

            entities_.pop_back();

            index_by_id_.erase(
                index_it);

            return true;
        }

        T* Find(
            EntityId id) noexcept
        {
            const auto it =
                index_by_id_.find(id);

            if (it ==
                index_by_id_.end())
            {
                return nullptr;
            }

            return &entities_[
                it->second
            ];
        }

        const T* Find(
            EntityId id) const noexcept
        {
            const auto it =
                index_by_id_.find(id);

            if (it ==
                index_by_id_.end())
            {
                return nullptr;
            }

            return &entities_[
                it->second
            ];
        }

        bool Contains(
            EntityId id) const noexcept
        {
            return
                index_by_id_.contains(id);
        }

        void Clear()
        {
            entities_.clear();
            index_by_id_.clear();
        }

        void Reserve(
            std::size_t capacity)
        {
            entities_.reserve(
                capacity);

            index_by_id_.reserve(
                capacity);
        }

        std::size_t size() const noexcept
        {
            return entities_.size();
        }

        bool empty() const noexcept
        {
            return entities_.empty();
        }

        Iterator begin() noexcept
        {
            return entities_.begin();
        }

        Iterator end() noexcept
        {
            return entities_.end();
        }

        ConstIterator begin() const noexcept
        {
            return entities_.begin();
        }

        ConstIterator end() const noexcept
        {
            return entities_.end();
        }

        ConstIterator cbegin() const noexcept
        {
            return entities_.cbegin();
        }

        ConstIterator cend() const noexcept
        {
            return entities_.cend();
        }

    private:
        // Dense authoritative entity storage.
        std::vector<T> entities_;

        // Derived lookup:
        //
        // EntityId → index inside entities_
        std::unordered_map<
            EntityId,
            std::size_t> index_by_id_;
    };
}