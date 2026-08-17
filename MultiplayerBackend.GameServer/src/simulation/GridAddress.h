#pragma once

#include <cstddef>

namespace simulation
{
    struct ChunkCoord
    {
        int x = 0;
        int y = 0;

        bool operator==(
            const ChunkCoord&) const noexcept = default;
    };

    struct LocalCellCoord
    {
        int x = 0;
        int y = 0;

        bool operator==(
            const LocalCellCoord&) const noexcept = default;
    };

    using CellIndex =
        std::size_t;

    struct GridAddress
    {
        ChunkCoord chunk;
        LocalCellCoord local;
        CellIndex index = 0;

        bool operator==(
            const GridAddress&) const noexcept = default;
    };
}