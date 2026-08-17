#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace protocol
{
    constexpr std::size_t PacketHeaderSize =
        sizeof(std::uint32_t) +
        sizeof(std::uint16_t);

    constexpr std::size_t MaxPayloadSize =
        64 * 1024;

    enum class PacketType : std::uint16_t
    {
        Ping = 1,
        Login = 2,
        Movement = 3
    };

    struct Packet
    {
        PacketType type;
        std::vector<std::byte> payload;
    };

    enum class DecodeStatus
    {
        Incomplete,
        Ready,
        Invalid
    };
}