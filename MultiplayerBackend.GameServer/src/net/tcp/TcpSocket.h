#pragma once

#include "../UniqueFd.h"

#include <cstddef>
#include <span>
#include <sys/types.h>

namespace net::tcp
{
    class TcpSocket
    {
    public:
        explicit TcpSocket(int fd = -1) noexcept;

        TcpSocket(const TcpSocket&) = delete;
        TcpSocket& operator=(const TcpSocket&) = delete;

        TcpSocket(TcpSocket&&) noexcept = default;
        TcpSocket& operator=(TcpSocket&&) noexcept = default;

        int Get() const noexcept;

        bool SetNonBlocking();

        ssize_t Send(std::span<const std::byte> data);
        ssize_t Receive(std::span<std::byte> buffer);

    private:
        UniqueFd fd_;
    };
}