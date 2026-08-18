using System.Text.Json;
using System.Security.Claims;
using MultiplayerBackend.Api.Data;
using MultiplayerBackend.Api.DTOs;
using Microsoft.EntityFrameworkCore;
using MultiplayerBackend.Api.Extensions;
using Microsoft.Extensions.Caching.Distributed;

namespace MultiplayerBackend.Api.Endpoints;

public static class PlayerEndpoints
{
    public static void MapPlayerEndpoints(this WebApplication app)
    {
        var group = app.MapGroup("/api/players");

        group.MapGet("/me", GetMe)
            .RequireAuthorization();

        group.MapPut("/me", UpdateMe)
            .RequireAuthorization();
    }

    private static async Task<IResult> GetMe(
        ClaimsPrincipal user,
        AppDbContext db,
        IDistributedCache cache,
        ILoggerFactory loggerFactory)
    {
        var playerId = user.GetPlayerId();

        if (playerId is null)
        {
            return Results.Unauthorized();
        }

        var logger = loggerFactory.CreateLogger("PlayerEndpoints");
        var cacheKey = $"player:{playerId.Value}";

        // 1. Try Redis first
        var cachedPlayer = await cache.GetStringAsync(cacheKey);

        if (cachedPlayer is not null)
        {
            logger.LogDebug(
                "Player cache hit for player {PlayerId}",
                playerId.Value);

            var response =
                JsonSerializer.Deserialize<PlayerResponse>(cachedPlayer);

            return Results.Ok(response);
        }

        logger.LogDebug(
            "Player cache miss for player {PlayerId}",
            playerId.Value);

        // 2. Redis didn't have it, query PostgreSQL
        var player = await db.Players
            .Where(player => player.Id == playerId.Value)
            .Select(player => new PlayerResponse(
                player.Id,
                player.Name,
                player.Level))
            .FirstOrDefaultAsync();

        if (player is null)
        {
            return Results.NotFound();
        }

        // 3. Store result in Redis
        var json = JsonSerializer.Serialize(player);

        await cache.SetStringAsync(
            cacheKey,
            json,
            new DistributedCacheEntryOptions
            {
                AbsoluteExpirationRelativeToNow =
                    TimeSpan.FromMinutes(5)
            });

        return Results.Ok(player);
    }

    private static async Task<IResult> UpdateMe(
        UpdatePlayerRequest request,
        ClaimsPrincipal user,
        AppDbContext db,
        IDistributedCache cache)
    {
        var playerId = user.GetPlayerId();

        if (playerId is null)
        {
            return Results.Unauthorized();
        }

        if (string.IsNullOrWhiteSpace(request.Name))
        {
            return Results.BadRequest("Name is required.");
        }

        if (request.Level < 0)
        {
            return Results.BadRequest(
                "Level cannot be negative.");
        }

        var player = await db.Players.FindAsync(playerId.Value);

        if (player is null)
        {
            return Results.NotFound();
        }

        player.Name = request.Name;
        player.Level = request.Level;

        await db.SaveChangesAsync();

        await cache.RemoveAsync($"player:{playerId.Value}");

        return Results.Ok(new PlayerResponse(
            player.Id,
            player.Name,
            player.Level));
    }
}