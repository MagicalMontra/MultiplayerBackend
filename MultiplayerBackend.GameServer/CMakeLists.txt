cmake_minimum_required(VERSION 3.20)

project(
    MultiplayerBackendGameServer
    LANGUAGES CXX
)

add_executable(
        multiplayer_game_server

        src/main.cpp

        src/net/UniqueFd.cpp
        src/net/tcp/TcpSocket.cpp

        src/protocol/PacketCodec.cpp
        src/protocol/PacketDecoder.cpp

        src/server/ClientConnection.cpp
        src/server/GameServer.cpp
        src/simulation/SimulationStep.h
        src/simulation/StateHistory.h
        src/simulation/WorldState.h
        src/simulation/MovementInput.h
        src/simulation/WorldSimulation.cpp
        src/simulation/WorldSimulation.h
        src/simulation/EntityId.h
        src/simulation/Vec2.h
        src/simulation/MoveDirection.h
        src/simulation/PlayerState.h
        src/simulation/CellCoord.h
        src/simulation/SpatialGrid.h
        src/simulation/SpatialGrid.cpp
        src/simulation/SimulationEvent.h
        src/simulation/EntityStore.h
        src/simulation/GridAddress.h
        src/replication/InterestTracker.h
        src/replication/InterestTracker.cpp
)

target_compile_features(
    multiplayer_game_server
    PRIVATE
    cxx_std_20
)

target_compile_options(
    multiplayer_game_server
    PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)