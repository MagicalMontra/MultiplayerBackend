using System.Text.Json;
using System.Security.Claims;
using MultiplayerBackend.Api.Data;
using MultiplayerBackend.Api.DTOs;
using MultiplayerBackend.Api.Models;
using Microsoft.EntityFrameworkCore;
using MultiplayerBackend.Api.Extensions;
using Microsoft.Extensions.Caching.Distributed;

namespace MultiplayerBackend.Api.Endpoints;

public static class PlayerEndpoints
{
    public static void MapPlayerEndpoints(this WebApplication app)
    {
        var group = app.MapGroup("/api/players");

        // Temporary dev endpoints
        group.MapGet("/", GetAll);
        group.MapPost("/", Create);
        group.MapDelete("/{id:int}", Delete);

        // Player-facing endpoints
        group.MapGet("/me", GetMe)
            .RequireAuthorization();

        group.MapPut("/me", UpdateMe)
            .RequireAuthorization();
    }

    private static async Task<IResult> GetAll(AppDbContext db)
    {
        var players = await db.Players
            .Select(player => new PlayerResponse(
                player.Id,
                player.Name,
                player.Level))
            .ToListAsync();

        return Results.Ok(players);
    }

    private static async Task<IResult> GetMe(
        ClaimsPrincipal user,
        AppDbContext db,
        IDistributedCache cache)
    {
        var playerId = user.GetPlayerId();

        if (playerId is null)
        {
            return Results.Unauthorized();
        }

        var cacheKey = $"player:{playerId.Value}";

        // 1. Try Redis first
        var cachedPlayer = await cache.GetStringAsync(cacheKey);

        if (cachedPlayer is not null)
        {
            Console.WriteLine("CACHE HIT");

            var response =
                JsonSerializer.Deserialize<PlayerResponse>(cachedPlayer);

            return Results.Ok(response);
        }

        Console.WriteLine("CACHE MISS");

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

    private static async Task<IResult> Create(
        CreatePlayerRequest request,
        AppDbContext db)
    {
        if (string.IsNullOrWhiteSpace(request.Name))
        {
            return Results.BadRequest("Name is required.");
        }

        if (request.Level < 0)
        {
            return Results.BadRequest("Level cannot be negative.");
        }

        var player = new Player
        {
            Name = request.Name,
            Level = request.Level
        };

        db.Players.Add(player);

        await db.SaveChangesAsync();

        var response = new PlayerResponse(
            player.Id,
            player.Name,
            player.Level);

        return Results.Created(
            $"/api/players/{player.Id}",
            response);
    }

    private static async Task<IResult> UpdateMe(
        UpdatePlayerRequest request,
        ClaimsPrincipal user,
        AppDbContext db)
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

        return Results.Ok(new PlayerResponse(
            player.Id,
            player.Name,
            player.Level));
    }

    private static async Task<IResult> Delete(
        int id,
        AppDbContext db)
    {
        var player = await db.Players.FindAsync(id);

        if (player is null)
        {
            return Results.NotFound();
        }

        db.Players.Remove(player);

        await db.SaveChangesAsync();

        return Results.NoContent();
    }
}