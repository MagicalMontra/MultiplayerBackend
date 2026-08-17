#include "server/GameServer.h"

int main()
{
    server::GameServer game_server{
        7777,
        12
    };

    return game_server.Run();
}