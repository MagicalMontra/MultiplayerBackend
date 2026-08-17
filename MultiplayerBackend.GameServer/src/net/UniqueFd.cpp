#include "UniqueFd.h"

#include <unistd.h>

namespace net
{
    UniqueFd::UniqueFd(int fd) noexcept
        : fd_(fd)
    {
    }

    UniqueFd::~UniqueFd()
    {
        Reset();
    }

    UniqueFd::UniqueFd(UniqueFd&& other) noexcept
        : fd_(other.fd_)
    {
        other.fd_ = -1;
    }

    UniqueFd& UniqueFd::operator=(UniqueFd&& other) noexcept
    {
        if (this != &other)
        {
            Reset();

            fd_ = other.fd_;
            other.fd_ = -1;
        }

        return *this;
    }

    int UniqueFd::Get() const noexcept
    {
        return fd_;
    }

    bool UniqueFd::IsValid() const noexcept
    {
        return fd_ != -1;
    }

    void UniqueFd::Reset(int fd) noexcept
    {
        if (fd_ != -1)
        {
            ::close(fd_);
        }

        fd_ = fd;
    }
}