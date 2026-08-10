namespace MultiplayerBackend.Api.DTOs;

public record CreatePlayerRequest(
    string Name,
    int Level
);

public record UpdatePlayerRequest(
    string Name,
    int Level
);

public record PlayerResponse(
    int Id,
    string Name,
    int Level
);