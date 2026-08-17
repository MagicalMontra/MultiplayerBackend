#include "TcpSocket.h"

#include <fcntl.h>
#include <sys/socket.h>

namespace net::tcp
{
    TcpSocket::TcpSocket(int fd) noexcept
        : fd_(fd)
    {
    }

    int TcpSocket::Get() const noexcept
    {
        return fd_.Get();
    }

    bool TcpSocket::SetNonBlocking()
    {
        const int flags =
            ::fcntl(fd_.Get(), F_GETFL, 0);

        if (flags == -1)
        {
            return false;
        }

        return ::fcntl(
            fd_.Get(),
            F_SETFL,
            flags | O_NONBLOCK) != -1;
    }

    ssize_t TcpSocket::Send(
        std::span<const std::byte> data)
    {
        return ::send(
            fd_.Get(),
            data.data(),
            data.size(),
            MSG_NOSIGNAL);
    }

    ssize_t TcpSocket::Receive(
        std::span<std::byte> buffer)
    {
        return ::recv(
            fd_.Get(),
            buffer.data(),
            buffer.size(),
            0);
    }
}