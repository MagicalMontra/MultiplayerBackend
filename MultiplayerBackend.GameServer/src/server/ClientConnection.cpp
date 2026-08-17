#include "ClientConnection.h"

#include <cerrno>
#include <utility>

namespace server
{
    ClientConnection::ClientConnection(
        net::tcp::TcpSocket socket)
        : socket_(std::move(socket))
    {
    }

    net::tcp::TcpSocket&
    ClientConnection::Socket()
    {
        return socket_;
    }

    protocol::PacketDecoder&
    ClientConnection::Decoder()
    {
        return decoder_;
    }

    void ClientConnection::QueueSend(
        std::span<const std::byte> data)
    {
        if (send_offset_ == send_buffer_.size())
        {
            send_buffer_.clear();
            send_offset_ = 0;
        }

        send_buffer_.insert(
            send_buffer_.end(),
            data.begin(),
            data.end());
    }

    FlushResult ClientConnection::FlushSend()
    {
        while (send_offset_ < send_buffer_.size())
        {
            const auto remaining =
                std::span<const std::byte>{
                    send_buffer_.data() + send_offset_,
                    send_buffer_.size() - send_offset_
                };

            const ssize_t sent =
                socket_.Send(remaining);

            if (sent > 0)
            {
                send_offset_ +=
                    static_cast<std::size_t>(sent);

                continue;
            }

            if (sent == -1)
            {
                if (errno == EAGAIN ||
                    errno == EWOULDBLOCK)
                {
                    return FlushResult::Pending;
                }

                if (errno == EINTR)
                {
                    continue;
                }

                return FlushResult::Error;
            }

            return FlushResult::Error;
        }

        send_buffer_.clear();
        send_offset_ = 0;

        return FlushResult::Complete;
    }

    bool ClientConnection::HasPendingSend() const
    {
        return send_offset_ < send_buffer_.size();
    }

    void ClientConnection::MarkReadClosed()
    {
        read_closed_ = true;
    }

    bool ClientConnection::IsReadClosed() const
    {
        return read_closed_;
    }
}