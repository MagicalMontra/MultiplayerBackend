#pragma once

#include "Packet.h"

#include <cstddef>
#include <span>
#include <vector>

namespace protocol
{
    class PacketDecoder
    {
    public:
        void Append(std::span<const std::byte> data);

        DecodeStatus TryPop(Packet& packet);

    private:
        void Compact();

        std::vector<std::byte> buffer_;
        std::size_t read_offset_ = 0;
    };
}