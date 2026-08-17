#include "SpatialGrid.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>

namespace simulation
{
    SpatialGrid::SpatialGrid(
        double cell_size)
        : cell_size_(cell_size)
    {
    }

    void SpatialGrid::Clear()
    {
        chunks_.clear();
        entity_cells_.clear();
    }

    void SpatialGrid::Insert(
        EntityId entity_id,
        Vec2 position)
    {
        // Prevent one entity from being indexed twice.
        Remove(entity_id);

        const CellCoord cell =
            ToCell(position);

        const GridAddress address =
            ToAddress(cell);

        Chunk& chunk =
            chunks_[address.chunk];

        chunk.cells[
            address.index
        ].push_back(
            entity_id);

        ++chunk.entity_count;

        entity_cells_[entity_id] =
            address;
    }

    void SpatialGrid::Remove(
        EntityId entity_id)
    {
        const auto entity_it =
            entity_cells_.find(
                entity_id);

        if (entity_it ==
            entity_cells_.end())
        {
            return;
        }

        const GridAddress address =
            entity_it->second;

        const auto chunk_it =
            chunks_.find(
                address.chunk);

        if (chunk_it !=
            chunks_.end())
        {
            Chunk& chunk =
                chunk_it->second;

            auto& entities =
                chunk.cells[
                    address.index
                ];

            const auto remove_it =
                std::find(
                    entities.begin(),
                    entities.end(),
                    entity_id);

            if (remove_it !=
                entities.end())
            {
                // Order inside a cell is irrelevant,
                // so use swap-and-pop rather than shifting
                // every following EntityId.
                *remove_it =
                    entities.back();

                entities.pop_back();

                --chunk.entity_count;
            }

            // Sparse world:
            //
            // once a chunk contains no entities at all,
            // remove the entire chunk allocation.
            if (chunk.entity_count == 0)
            {
                chunks_.erase(
                    chunk_it);
            }
        }

        entity_cells_.erase(
            entity_it);
    }

    void SpatialGrid::Update(
        EntityId entity_id,
        Vec2 position)
    {
        const CellCoord new_cell =
            ToCell(position);

        const GridAddress new_address =
            ToAddress(new_cell);

        const auto entity_it =
            entity_cells_.find(
                entity_id);

        if (entity_it ==
            entity_cells_.end())
        {
            Insert(
                entity_id,
                position);

            return;
        }

        const GridAddress old_address =
            entity_it->second;

        // Still inside the same cell.
        if (old_address ==
            new_address)
        {
            return;
        }

        // -------------------------------------------------
        // Remove from old cell.
        // -------------------------------------------------

        const auto old_chunk_it =
            chunks_.find(
                old_address.chunk);

        if (old_chunk_it !=
            chunks_.end())
        {
            Chunk& old_chunk =
                old_chunk_it->second;

            auto& old_entities =
                old_chunk.cells[
                    old_address.index
                ];

            const auto old_entity_it =
                std::find(
                    old_entities.begin(),
                    old_entities.end(),
                    entity_id);

            if (old_entity_it !=
                old_entities.end())
            {
                *old_entity_it =
                    old_entities.back();

                old_entities.pop_back();

                --old_chunk.entity_count;
            }

            if (old_chunk.entity_count == 0)
            {
                chunks_.erase(
                    old_chunk_it);
            }
        }

        // -------------------------------------------------
        // Insert into new cell.
        // -------------------------------------------------

        Chunk& new_chunk =
            chunks_[new_address.chunk];

        new_chunk.cells[
            new_address.index
        ].push_back(
            entity_id);

        ++new_chunk.entity_count;

        // Update the entity's cached spatial location.
        entity_it->second =
            new_address;
    }

    CellCoord SpatialGrid::ToCell(
        Vec2 position) const noexcept
    {
        return CellCoord{
            .x =
                static_cast<int>(
                    std::floor(
                        position.x /
                        cell_size_)),

            .y =
                static_cast<int>(
                    std::floor(
                        position.y /
                        cell_size_))
        };
    }

    GridAddress SpatialGrid::ToAddress(
        CellCoord cell) const noexcept
    {
        const ChunkCoord chunk{
            .x =
                FloorDiv(
                    cell.x,
                    ChunkWidth),

            .y =
                FloorDiv(
                    cell.y,
                    ChunkHeight)
        };

        const LocalCellCoord local{
            .x =
                PositiveModulo(
                    cell.x,
                    ChunkWidth),

            .y =
                PositiveModulo(
                    cell.y,
                    ChunkHeight)
        };

        return GridAddress{
            .chunk = chunk,
            .local = local,
            .index =
                ToCellIndex(
                    local)
        };
    }

    CellIndex SpatialGrid::ToCellIndex(
        LocalCellCoord local) noexcept
    {
        return
            static_cast<CellIndex>(
                local.y) *
            static_cast<CellIndex>(
                ChunkWidth)
            +
            static_cast<CellIndex>(
                local.x);
    }

    LocalCellCoord
    SpatialGrid::FromCellIndex(
        CellIndex index) noexcept
    {
        return LocalCellCoord{
            .x =
                static_cast<int>(
                    index %
                    static_cast<CellIndex>(
                        ChunkWidth)),

            .y =
                static_cast<int>(
                    index /
                    static_cast<CellIndex>(
                        ChunkWidth))
        };
    }

    std::vector<EntityId>
    SpatialGrid::QueryNeighborhood(
        CellCoord center,
        int cell_radius) const
    {
        std::vector<EntityId> result;

        if (cell_radius < 0)
        {
            return result;
        }

        for (int y =
                 center.y -
                 cell_radius;
             y <=
                 center.y +
                 cell_radius;
             ++y)
        {
            for (int x =
                     center.x -
                     cell_radius;
                 x <=
                     center.x +
                     cell_radius;
                 ++x)
            {
                const CellCoord global_cell{
                    .x = x,
                    .y = y
                };

                const GridAddress address =
                    ToAddress(
                        global_cell);

                const auto chunk_it =
                    chunks_.find(
                        address.chunk);

                if (chunk_it ==
                    chunks_.end())
                {
                    continue;
                }

                const auto& entities =
                    chunk_it
                        ->second
                        .cells[
                            address.index
                        ];

                result.insert(
                    result.end(),
                    entities.begin(),
                    entities.end());
            }
        }

        // Internal hash-map/chunk storage order should not
        // affect externally visible replication ordering.
        std::sort(
            result.begin(),
            result.end());

        return result;
    }

    std::vector<EntityId>
    SpatialGrid::QueryNeighborhood(
        Vec2 position,
        int cell_radius) const
    {
        return QueryNeighborhood(
            ToCell(position),
            cell_radius);
    }

    double SpatialGrid::CellSize() const noexcept
    {
        return cell_size_;
    }

    std::size_t
    SpatialGrid::ChunkCount() const noexcept
    {
        return chunks_.size();
    }

    int SpatialGrid::FloorDiv(
        int value,
        int divisor) noexcept
    {
        // C++ integer division truncates toward zero.
        //
        // For spatial coordinates we need mathematical
        // floor division.
        //
        // Example:
        //
        // -17 / 16
        //
        // C++:
        //     -1
        //
        // Spatial floor division:
        //     -2

        int quotient =
            value /
            divisor;

        const int remainder =
            value %
            divisor;

        if (remainder < 0)
        {
            --quotient;
        }

        return quotient;
    }

    int SpatialGrid::PositiveModulo(
        int value,
        int divisor) noexcept
    {
        int remainder =
            value %
            divisor;

        if (remainder < 0)
        {
            remainder +=
                divisor;
        }

        return remainder;
    }

    std::size_t
    SpatialGrid::ChunkCoordHash::operator()(
        const ChunkCoord& coord) const noexcept
    {
        const auto x =
            static_cast<std::uint32_t>(
                coord.x);

        const auto y =
            static_cast<std::uint32_t>(
                coord.y);

        const std::uint64_t combined =
            (
                static_cast<std::uint64_t>(x)
                << 32
            )
            |
            static_cast<std::uint64_t>(y);

        return
            std::hash<
                std::uint64_t>{}(
                    combined);
    }
}