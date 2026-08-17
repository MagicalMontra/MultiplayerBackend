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
        src/simulation/SimulationStep.cpp
        src/simulation/SimulationStep.h
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