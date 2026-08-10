using MultiplayerBackend.Api.DTOs;
using MultiplayerBackend.Api.Services;

namespace MultiplayerBackend.Api.Endpoints;

public static class AuthEndpoints
{
    public static void MapAuthEndpoints(
        this WebApplication app)
    {
        var group = app.MapGroup("/api/auth");

        group.MapPost("/register", Register);
        group.MapPost("/login", Login);
    }

    private static async Task<IResult> Register(
        RegisterRequest request,
        AuthService authService)
    {
        if (string.IsNullOrWhiteSpace(request.Username))
        {
            return Results.BadRequest(
                "Username is required.");
        }

        if (string.IsNullOrWhiteSpace(request.Password))
        {
            return Results.BadRequest(
                "Password is required.");
        }

        if (string.IsNullOrWhiteSpace(request.PlayerName))
        {
            return Results.BadRequest(
                "Player name is required.");
        }

        var result =
            await authService.RegisterAsync(request);

        if (result is null)
        {
            return Results.Conflict(
                "Username already exists.");
        }

        return Results.Created(
            $"/api/players/{result.PlayerId}",
            result);
    }
    
    private static async Task<IResult> Login(
        LoginRequest request,
        AuthService authService)
    {
        if (string.IsNullOrWhiteSpace(request.Username) ||
            string.IsNullOrWhiteSpace(request.Password))
        {
            return Results.BadRequest(
                "Username and password are required.");
        }

        var result = await authService.LoginAsync(request);

        if (result is null)
        {
            return Results.Unauthorized();
        }

        return Results.Ok(result);
    }
}