#include "PacketCodec.h"

#include <arpa/inet.h>
#include <cstring>

namespace protocol
{
    std::vector<std::byte> EncodePacket(
        PacketType type,
        std::span<const std::byte> payload)
    {
        const auto payload_size =
            static_cast<std::uint32_t>(payload.size());

        const auto network_size =
            htonl(payload_size);

        const auto network_type =
            htons(static_cast<std::uint16_t>(type));

        std::vector<std::byte> packet(
            PacketHeaderSize + payload.size());

        std::memcpy(
            packet.data(),
            &network_size,
            sizeof(network_size));

        std::memcpy(
            packet.data() + sizeof(network_size),
            &network_type,
            sizeof(network_type));

        std::memcpy(
            packet.data() + PacketHeaderSize,
            payload.data(),
            payload.size());

        return packet;
    }
}