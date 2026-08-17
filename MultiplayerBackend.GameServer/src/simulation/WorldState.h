#pragma once

#include "EntityId.h"
#include "EntityStore.h"
#include "PlayerState.h"

namespace simulation
{
    struct WorldState
    {
        EntityStore<PlayerState>
            players;

        PlayerState* FindPlayer(
            EntityId id) noexcept
        {
            return players.Find(id);
        }

        const PlayerState* FindPlayer(
            EntityId id) const noexcept
        {
            return players.Find(id);
        }

        bool SpawnPlayer(
            const PlayerState& player)
        {
            return players.Insert(
                player);
        }

        bool DespawnEntity(
            EntityId id)
        {
            return players.Remove(id);
        }
    };
}