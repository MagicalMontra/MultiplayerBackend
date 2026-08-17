#pragma once

#include "CellCoord.h"
#include "EntityId.h"
#include "GridAddress.h"
#include "Vec2.h"

#include <array>
#include <cstddef>
#include <unordered_map>
#include <vector>

namespace simulation
{
    class SpatialGrid
    {
    public:
        static constexpr int ChunkWidth =
            16;

        static constexpr int ChunkHeight =
            16;

        static constexpr std::size_t
        CellsPerChunk =
            static_cast<std::size_t>(
                ChunkWidth *
                ChunkHeight);

        explicit SpatialGrid(
            double cell_size);

        void Clear();

        void Insert(
            EntityId entity_id,
            Vec2 position);

        void Remove(
            EntityId entity_id);

        void Update(
            EntityId entity_id,
            Vec2 position);

        CellCoord ToCell(
            Vec2 position) const noexcept;

        GridAddress ToAddress(
            CellCoord cell) const noexcept;

        static CellIndex ToCellIndex(
            LocalCellCoord local) noexcept;

        static LocalCellCoord FromCellIndex(
            CellIndex index) noexcept;

        std::vector<EntityId>
        QueryNeighborhood(
            CellCoord center,
            int cell_radius) const;

        std::vector<EntityId>
        QueryNeighborhood(
            Vec2 position,
            int cell_radius) const;

        double CellSize() const noexcept;

        std::size_t ChunkCount() const noexcept;

    private:
        struct ChunkCoordHash
        {
            std::size_t operator()(
                const ChunkCoord& coord) const noexcept;
        };

        struct Chunk
        {
            // Conceptually:
            //
            // Cell[x][y]
            //
            // Physically:
            //
            // cells[y * ChunkWidth + x]
            std::array<
                std::vector<EntityId>,
                CellsPerChunk> cells;

            std::size_t entity_count = 0;
        };

        static int FloorDiv(
            int value,
            int divisor) noexcept;

        static int PositiveModulo(
            int value,
            int divisor) noexcept;

        double cell_size_;

        // Only chunks which actually contain entities
        // need to exist.
        std::unordered_map<
            ChunkCoord,
            Chunk,
            ChunkCoordHash> chunks_;

        // Entity -> its current spatial address.
        //
        // This lets Update/Remove avoid searching the
        // whole world.
        std::unordered_map<
            EntityId,
            GridAddress> entity_cells_;
    };
}