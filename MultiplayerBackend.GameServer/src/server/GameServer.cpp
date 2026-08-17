#include "GameServer.h"

#include "../protocol/PacketCodec.h"

#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <span>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <utility>

namespace server
{
    GameServer::GameServer(
        std::uint16_t port,
        std::uint32_t tick_rate)
        : port_(port),
          tick_rate_(tick_rate)
    {
    }

    int GameServer::Run()
    {
        if (!InitializeListener())
        {
            return 1;
        }

        if (!InitializeEpoll())
        {
            return 1;
        }

        if (!InitializeTickTimer())
        {
            return 1;
        }

        std::cout
            << "Listening on port "
            << port_
            << "...\n";

        std::cout
            << "Game tick rate: "
            << tick_rate_
            << " Hz\n";

        while (true)
        {
            const int event_count =
                ::epoll_wait(
                    epoll_.Get(),
                    events_.data(),
                    static_cast<int>(events_.size()),
                    -1);

            if (event_count == -1)
            {
                if (errno == EINTR)
                {
                    continue;
                }

                std::cerr
                    << "epoll_wait() failed: "
                    << std::strerror(errno)
                    << '\n';

                return 1;
            }

            for (int i = 0; i < event_count; ++i)
            {
                const int fd =
                    events_[i].data.fd;

                const std::uint32_t ready_events =
                    events_[i].events;

                // ---------------------------------------------
                // Listening socket.
                // ---------------------------------------------

                if (fd == listener_.Get())
                {
                    AcceptPendingClients();
                    continue;
                }

                // ---------------------------------------------
                // Authoritative simulation timer.
                // ---------------------------------------------

                if (fd == tick_timer_.Get())
                {
                    if (!ProcessTickTimer())
                    {
                        return 1;
                    }

                    continue;
                }

                // ---------------------------------------------
                // Client socket.
                // ---------------------------------------------

                HandleClientEvent(
                    fd,
                    ready_events);
            }
        }
    }

    bool GameServer::InitializeListener()
    {
        const int listener_fd =
            ::socket(
                AF_INET,
                SOCK_STREAM,
                0);

        if (listener_fd == -1)
        {
            std::cerr
                << "socket() failed: "
                << std::strerror(errno)
                << '\n';

            return false;
        }

        listener_ =
            net::tcp::TcpSocket{
                listener_fd
            };

        int reuse = 1;

        if (::setsockopt(
                listener_.Get(),
                SOL_SOCKET,
                SO_REUSEADDR,
                &reuse,
                sizeof(reuse)) == -1)
        {
            std::cerr
                << "setsockopt() failed: "
                << std::strerror(errno)
                << '\n';

            return false;
        }

        sockaddr_in address{};

        address.sin_family = AF_INET;
        address.sin_addr.s_addr =
            htonl(INADDR_ANY);
        address.sin_port =
            htons(port_);

        if (::bind(
                listener_.Get(),
                reinterpret_cast<sockaddr*>(&address),
                sizeof(address)) == -1)
        {
            std::cerr
                << "bind() failed: "
                << std::strerror(errno)
                << '\n';

            return false;
        }

        if (::listen(
                listener_.Get(),
                128) == -1)
        {
            std::cerr
                << "listen() failed: "
                << std::strerror(errno)
                << '\n';

            return false;
        }

        if (!listener_.SetNonBlocking())
        {
            std::cerr
                << "Failed to make listener non-blocking: "
                << std::strerror(errno)
                << '\n';

            return false;
        }

        return true;
    }

    bool GameServer::InitializeEpoll()
    {
        epoll_ =
            net::UniqueFd{
                ::epoll_create1(
                    EPOLL_CLOEXEC)
            };

        if (!epoll_.IsValid())
        {
            std::cerr
                << "epoll_create1() failed: "
                << std::strerror(errno)
                << '\n';

            return false;
        }

        epoll_event listener_event{};

        listener_event.events =
            EPOLLIN;

        listener_event.data.fd =
            listener_.Get();

        if (::epoll_ctl(
                epoll_.Get(),
                EPOLL_CTL_ADD,
                listener_.Get(),
                &listener_event) == -1)
        {
            std::cerr
                << "Failed to add listener to epoll: "
                << std::strerror(errno)
                << '\n';

            return false;
        }

        return true;
    }

    bool GameServer::InitializeTickTimer()
    {
        constexpr std::uint64_t NanosecondsPerSecond =
            1'000'000'000ULL;

        if (tick_rate_ == 0 ||
            tick_rate_ > NanosecondsPerSecond)
        {
            std::cerr
                << "Invalid tick rate: "
                << tick_rate_
                << '\n';

            return false;
        }

        tick_timer_ =
            net::UniqueFd{
                ::timerfd_create(
                    CLOCK_MONOTONIC,
                    TFD_NONBLOCK |
                    TFD_CLOEXEC)
            };

        if (!tick_timer_.IsValid())
        {
            std::cerr
                << "timerfd_create() failed: "
                << std::strerror(errno)
                << '\n';

            return false;
        }

        const std::uint64_t tick_nanoseconds =
            NanosecondsPerSecond /
            tick_rate_;

        itimerspec timer_spec{};

        timer_spec.it_value.tv_sec =
            static_cast<time_t>(
                tick_nanoseconds /
                NanosecondsPerSecond);

        timer_spec.it_value.tv_nsec =
            static_cast<long>(
                tick_nanoseconds %
                NanosecondsPerSecond);

        timer_spec.it_interval =
            timer_spec.it_value;

        if (::timerfd_settime(
                tick_timer_.Get(),
                0,
                &timer_spec,
                nullptr) == -1)
        {
            std::cerr
                << "timerfd_settime() failed: "
                << std::strerror(errno)
                << '\n';

            return false;
        }

        epoll_event timer_event{};

        timer_event.events =
            EPOLLIN;

        timer_event.data.fd =
            tick_timer_.Get();

        if (::epoll_ctl(
                epoll_.Get(),
                EPOLL_CTL_ADD,
                tick_timer_.Get(),
                &timer_event) == -1)
        {
            std::cerr
                << "Failed to add tick timer to epoll: "
                << std::strerror(errno)
                << '\n';

            return false;
        }

        return true;
    }

    bool GameServer::ProcessTickTimer()
    {
        std::uint64_t expirations = 0;

        while (true)
        {
            const ssize_t bytes_read =
                ::read(
                    tick_timer_.Get(),
                    &expirations,
                    sizeof(expirations));

            if (bytes_read ==
                static_cast<ssize_t>(
                    sizeof(expirations)))
            {
                break;
            }

            if (bytes_read == -1 &&
                errno == EINTR)
            {
                continue;
            }

            if (bytes_read == -1 &&
                (errno == EAGAIN ||
                 errno == EWOULDBLOCK))
            {
                return true;
            }

            std::cerr
                << "Failed to read tick timer: "
                << std::strerror(errno)
                << '\n';

            return false;
        }

        // If more than one expiration occurred, the server
        // fell behind. We advance one authoritative
        // simulation step for each missed expiration.
        for (std::uint64_t i = 0;
             i < expirations;
             ++i)
        {
            const auto step =
                AdvanceSimulationClock();

            Tick(step);
        }

        return true;
    }

    simulation::SimulationStep
    GameServer::AdvanceSimulationClock()
    {
        const simulation::GameTime start_time =
            TimeAtTick(tick_count_);

        ++tick_count_;

        const simulation::GameTime end_time =
            TimeAtTick(tick_count_);

        return simulation::SimulationStep{
            .tick = tick_count_,
            .start_time = start_time,
            .end_time = end_time,
            .delta_time =
                end_time - start_time
        };
    }

    simulation::GameTime
    GameServer::TimeAtTick(
        std::uint64_t tick) const
    {
        constexpr std::uint64_t NanosecondsPerSecond =
            1'000'000'000ULL;

        const std::uint64_t whole_seconds =
            tick /
            tick_rate_;

        const std::uint64_t remainder_ticks =
            tick %
            tick_rate_;

        const std::uint64_t remainder_nanoseconds =
            (
                remainder_ticks *
                NanosecondsPerSecond
            ) /
            tick_rate_;

        return
            std::chrono::seconds{
                static_cast<std::int64_t>(
                    whole_seconds)
            }
            +
            std::chrono::nanoseconds{
                static_cast<std::int64_t>(
                    remainder_nanoseconds)
            };
    }

    void GameServer::Tick(
        const simulation::SimulationStep& step)
    {
        const double delta_time =
            step.DeltaSeconds();

        // We will use this as soon as actual gameplay
        // simulation is introduced.
        (void)delta_time;

        // Temporary diagnostic output.
        //
        // At 12 Hz this prints once every 12 ticks.
        if (step.tick % tick_rate_ == 0)
        {
            const double game_time_seconds =
                std::chrono::duration<double>(
                    step.end_time).count();

            std::cout
                << "Tick "
                << step.tick
                << " | time="
                << game_time_seconds
                << "s"
                << " | clients="
                << clients_.size()
                << '\n';
        }

        // Eventually this becomes roughly:
        //
        // ProcessInputs(step);
        // SimulateWorld(step);
        // ResolveCombat(step);
        // SaveRollbackState(step);
        // BuildSnapshots(step);
    }

    void GameServer::AcceptPendingClients()
    {
        while (true)
        {
            const int client_fd =
                ::accept(
                    listener_.Get(),
                    nullptr,
                    nullptr);

            if (client_fd == -1)
            {
                if (errno == EAGAIN ||
                    errno == EWOULDBLOCK)
                {
                    return;
                }

                if (errno == EINTR)
                {
                    continue;
                }

                std::cerr
                    << "accept() failed: "
                    << std::strerror(errno)
                    << '\n';

                return;
            }

            net::tcp::TcpSocket client{
                client_fd
            };

            if (!client.SetNonBlocking())
            {
                std::cerr
                    << "Failed to make client non-blocking: "
                    << std::strerror(errno)
                    << '\n';

                continue;
            }

            epoll_event client_event{};

            client_event.events =
                EPOLLIN |
                EPOLLRDHUP;

            client_event.data.fd =
                client_fd;

            if (::epoll_ctl(
                    epoll_.Get(),
                    EPOLL_CTL_ADD,
                    client_fd,
                    &client_event) == -1)
            {
                std::cerr
                    << "Failed to add client to epoll: "
                    << std::strerror(errno)
                    << '\n';

                continue;
            }

            clients_.try_emplace(
                client_fd,
                std::move(client));

            std::cout
                << "Client connected: fd="
                << client_fd
                << '\n';
        }
    }

    void GameServer::HandleClientEvent(
        int fd,
        std::uint32_t ready_events)
    {
        const auto client_it =
            clients_.find(fd);

        if (client_it == clients_.end())
        {
            return;
        }

        auto& connection =
            client_it->second;

        bool disconnected = false;

        // -------------------------------------------------
        // Existing queued output became writable.
        // -------------------------------------------------

        if (ready_events & EPOLLOUT)
        {
            const auto result =
                connection.FlushSend();

            if (result ==
                FlushResult::Error)
            {
                std::cerr
                    << "send() failed for fd="
                    << fd
                    << ": "
                    << std::strerror(errno)
                    << '\n';

                disconnected = true;
            }
        }

        // -------------------------------------------------
        // Incoming data.
        // -------------------------------------------------

        if (!disconnected &&
            (ready_events & EPOLLIN))
        {
            while (true)
            {
                const ssize_t received =
                    connection
                        .Socket()
                        .Receive(
                            receive_buffer_);

                if (received > 0)
                {
                    connection
                        .Decoder()
                        .Append(
                            std::span{
                                receive_buffer_.data(),
                                static_cast<std::size_t>(
                                    received)
                            });

                    while (true)
                    {
                        protocol::Packet packet;

                        const auto status =
                            connection
                                .Decoder()
                                .TryPop(packet);

                        if (status ==
                            protocol::DecodeStatus::Incomplete)
                        {
                            break;
                        }

                        if (status ==
                            protocol::DecodeStatus::Invalid)
                        {
                            std::cerr
                                << "Invalid packet from fd="
                                << fd
                                << '\n';

                            disconnected = true;
                            break;
                        }

                        std::cout
                            << "Packet from fd="
                            << fd
                            << " type="
                            << static_cast<std::uint16_t>(
                                   packet.type)
                            << " payload="
                            << packet.payload.size()
                            << " bytes\n";

                        if (packet.type ==
                            protocol::PacketType::Ping)
                        {
                            const auto response =
                                protocol::EncodePacket(
                                    protocol::PacketType::Ping,
                                    packet.payload);

                            connection.QueueSend(
                                response);
                        }
                    }

                    if (disconnected)
                    {
                        break;
                    }

                    continue;
                }

                if (received == 0)
                {
                    connection.MarkReadClosed();
                    break;
                }

                if (errno == EAGAIN ||
                    errno == EWOULDBLOCK)
                {
                    break;
                }

                if (errno == EINTR)
                {
                    continue;
                }

                std::cerr
                    << "recv() failed for fd="
                    << fd
                    << ": "
                    << std::strerror(errno)
                    << '\n';

                disconnected = true;
                break;
            }
        }

        // -------------------------------------------------
        // Peer closed its writing half.
        // -------------------------------------------------

        if (!disconnected &&
            (ready_events & EPOLLRDHUP))
        {
            connection.MarkReadClosed();
        }

        // -------------------------------------------------
        // Try immediately flushing newly queued output.
        // -------------------------------------------------

        if (!disconnected &&
            connection.HasPendingSend())
        {
            const auto result =
                connection.FlushSend();

            if (result ==
                FlushResult::Error)
            {
                std::cerr
                    << "send() failed for fd="
                    << fd
                    << ": "
                    << std::strerror(errno)
                    << '\n';

                disconnected = true;
            }
        }

        // -------------------------------------------------
        // Peer finished sending and all our outgoing data
        // has been delivered.
        // -------------------------------------------------

        if (!disconnected &&
            connection.IsReadClosed() &&
            !connection.HasPendingSend())
        {
            disconnected = true;
        }

        // -------------------------------------------------
        // Update epoll interest.
        // -------------------------------------------------

        if (!disconnected)
        {
            const bool want_read =
                !connection.IsReadClosed();

            const bool want_write =
                connection.HasPendingSend();

            if (!UpdateClientInterest(
                    fd,
                    want_read,
                    want_write))
            {
                std::cerr
                    << "Failed to update epoll interest for fd="
                    << fd
                    << ": "
                    << std::strerror(errno)
                    << '\n';

                disconnected = true;
            }
        }

        if (disconnected)
        {
            DisconnectClient(fd);
        }
    }

    bool GameServer::UpdateClientInterest(
        int fd,
        bool want_read,
        bool want_write)
    {
        epoll_event event{};

        if (want_read)
        {
            event.events |=
                EPOLLIN |
                EPOLLRDHUP;
        }

        if (want_write)
        {
            event.events |=
                EPOLLOUT;
        }

        event.data.fd = fd;

        return ::epoll_ctl(
            epoll_.Get(),
            EPOLL_CTL_MOD,
            fd,
            &event) != -1;
    }

    void GameServer::DisconnectClient(
        int fd)
    {
        ::epoll_ctl(
            epoll_.Get(),
            EPOLL_CTL_DEL,
            fd,
            nullptr);

        clients_.erase(fd);

        std::cout
            << "Client disconnected: fd="
            << fd
            << '\n';
    }
}