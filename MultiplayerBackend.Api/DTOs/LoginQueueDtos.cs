namespace MultiplayerBackend.Api.DTOs;

public record LoginQueueStatusResponse(
    bool InQueue,
    long? Position,
    long TotalPlayers
);