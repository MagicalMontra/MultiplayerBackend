#pragma once

#include "../net/tcp/TcpSocket.h"
#include "../protocol/PacketDecoder.h"

#include <cstddef>
#include <span>
#include <vector>

namespace server
{
    enum class FlushResult
    {
        Complete,
        Pending,
        Error
    };

    class ClientConnection
    {
    public:
        explicit ClientConnection(
            net::tcp::TcpSocket socket);

        net::tcp::TcpSocket& Socket();
        protocol::PacketDecoder& Decoder();

        void QueueSend(
            std::span<const std::byte> data);

        FlushResult FlushSend();

        bool HasPendingSend() const;

        void MarkReadClosed();
        bool IsReadClosed() const;

    private:
        net::tcp::TcpSocket socket_;
        protocol::PacketDecoder decoder_;

        std::vector<std::byte> send_buffer_;
        std::size_t send_offset_ = 0;

        bool read_closed_ = false;
    };
}