#pragma once

#include "Packet.h"

#include <cstddef>
#include <span>
#include <vector>

namespace protocol
{
    std::vector<std::byte> EncodePacket(
        PacketType type,
        std::span<const std::byte> payload);
}