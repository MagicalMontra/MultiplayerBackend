#pragma once

#include "../net/UniqueFd.h"
#include "../net/tcp/TcpSocket.h"
#include "../replication/InterestTracker.h"
#include "../simulation/SimulationStep.h"
#include "../simulation/WorldSimulation.h"
#include "ClientConnection.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <sys/epoll.h>
#include <unordered_map>

namespace server
{
    class GameServer
    {
    public:
        explicit GameServer(
            std::uint16_t port,
            std::uint32_t tick_rate = 30);

        int Run();

    private:
        static constexpr int MaxEvents = 64;

        static constexpr std::size_t
        ReceiveBufferSize = 4096;

        bool InitializeListener();
        bool InitializeEpoll();
        bool InitializeTickTimer();

        bool ProcessTickTimer();

        simulation::SimulationStep
        AdvanceSimulationClock();

        simulation::GameTime
        TimeAtTick(
            std::uint64_t tick) const;

        void Tick(
            const simulation::SimulationStep& step);

        void AcceptPendingClients();

        void HandleClientEvent(
            int fd,
            std::uint32_t ready_events);

        bool UpdateClientInterest(
            int fd,
            bool want_read,
            bool want_write);

        void DisconnectClient(
            int fd);

        std::uint16_t port_;
        std::uint32_t tick_rate_;

        net::tcp::TcpSocket listener_;
        net::UniqueFd epoll_;
        net::UniqueFd tick_timer_;

        std::unordered_map<
            int,
            ClientConnection> clients_;

        std::array<
            epoll_event,
            MaxEvents> events_{};

        std::array<
            std::byte,
            ReceiveBufferSize> receive_buffer_{};

        simulation::WorldSimulation
            world_simulation_;

        std::unordered_map<
            simulation::EntityId,
            replication::InterestTracker>
            interest_by_observer_;

        std::uint64_t tick_count_ = 0;

        bool rollback_demo_submitted_ =
            false;

        bool lifecycle_demo_submitted_ =
            false;
    };
}