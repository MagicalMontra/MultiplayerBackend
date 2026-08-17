#include "PacketDecoder.h"

#include <arpa/inet.h>
#include <algorithm>
#include <cstring>

namespace protocol
{
    void PacketDecoder::Append(
        std::span<const std::byte> data)
    {
        // Reclaim consumed space occasionally rather than
        // shifting the vector after every packet.
        if (read_offset_ >= 4096 ||
            (read_offset_ > 0 &&
             read_offset_ * 2 >= buffer_.size()))
        {
            Compact();
        }

        buffer_.insert(
            buffer_.end(),
            data.begin(),
            data.end());
    }

    DecodeStatus PacketDecoder::TryPop(Packet& packet)
    {
        const std::size_t available =
            buffer_.size() - read_offset_;

        if (available < PacketHeaderSize)
        {
            return DecodeStatus::Incomplete;
        }

        const std::byte* packet_begin =
            buffer_.data() + read_offset_;

        std::uint32_t network_payload_size{};
        std::uint16_t network_packet_type{};

        std::memcpy(
            &network_payload_size,
            packet_begin,
            sizeof(network_payload_size));

        std::memcpy(
            &network_packet_type,
            packet_begin + sizeof(network_payload_size),
            sizeof(network_packet_type));

        const std::uint32_t payload_size =
            ntohl(network_payload_size);

        const std::uint16_t packet_type =
            ntohs(network_packet_type);

        if (payload_size > MaxPayloadSize)
        {
            return DecodeStatus::Invalid;
        }

        const std::size_t packet_size =
            PacketHeaderSize + payload_size;

        if (available < packet_size)
        {
            return DecodeStatus::Incomplete;
        }

        packet.type =
            static_cast<PacketType>(packet_type);

        const auto payload_begin =
            buffer_.begin() +
            static_cast<std::ptrdiff_t>(
                read_offset_ + PacketHeaderSize);

        const auto payload_end =
            buffer_.begin() +
            static_cast<std::ptrdiff_t>(
                read_offset_ + packet_size);

        packet.payload.assign(
            payload_begin,
            payload_end);

        // No erase here.
        read_offset_ += packet_size;

        // Everything was consumed.
        if (read_offset_ == buffer_.size())
        {
            buffer_.clear();
            read_offset_ = 0;
        }

        return DecodeStatus::Ready;
    }

    void PacketDecoder::Compact()
    {
        if (read_offset_ == 0)
        {
            return;
        }

        if (read_offset_ == buffer_.size())
        {
            buffer_.clear();
            read_offset_ = 0;
            return;
        }

        std::move(
            buffer_.begin() +
                static_cast<std::ptrdiff_t>(read_offset_),
            buffer_.end(),
            buffer_.begin());

        buffer_.resize(
            buffer_.size() - read_offset_);

        read_offset_ = 0;
    }
}