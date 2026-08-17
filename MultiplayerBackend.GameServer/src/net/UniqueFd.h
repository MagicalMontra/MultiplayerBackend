#pragma once

namespace net
{
    class UniqueFd
    {
    public:
        explicit UniqueFd(int fd = -1) noexcept;
        ~UniqueFd();

        UniqueFd(const UniqueFd&) = delete;
        UniqueFd& operator=(const UniqueFd&) = delete;

        UniqueFd(UniqueFd&& other) noexcept;
        UniqueFd& operator=(UniqueFd&& other) noexcept;

        int Get() const noexcept;
        bool IsValid() const noexcept;

        void Reset(int fd = -1) noexcept;

    private:
        int fd_;
    };
}