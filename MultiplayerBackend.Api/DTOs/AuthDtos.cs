namespace MultiplayerBackend.Api.DTOs;

public record RegisterRequest(
    string Username,
    string Password,
    string PlayerName
);

public record RegisterResponse(
    int AccountId,
    int PlayerId,
    string Username,
    string PlayerName
);

public record LoginRequest(
    string Username,
    string Password
);

public record LoginResponse(
    int AccountId,
    int PlayerId,
    string Username
);