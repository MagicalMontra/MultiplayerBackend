#pragma once

namespace simulation
{
    struct CellCoord
    {
        int x = 0;
        int y = 0;

        bool operator==(
            const CellCoord&) const noexcept = default;
    };
}