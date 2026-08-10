using Microsoft.EntityFrameworkCore;
using MultiplayerBackend.Api.Data;
using MultiplayerBackend.Api.DTOs;
using MultiplayerBackend.Api.Models;

namespace MultiplayerBackend.Api.Endpoints;

public static class PlayerEndpoints
{
    public static void MapPlayerEndpoints(this WebApplication app)
    {
        var group = app.MapGroup("/api/players");

        group.MapGet("/", GetAll);
        group.MapGet("/{id:int}", GetById);
        group.MapPost("/", Create);
        group.MapPut("/{id:int}", Update);
        group.MapDelete("/{id:int}", Delete);
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

    private static async Task<IResult> GetById(
        int id,
        AppDbContext db)
    {
        var player = await db.Players
            .Where(player => player.Id == id)
            .Select(player => new PlayerResponse(
                player.Id,
                player.Name,
                player.Level))
            .FirstOrDefaultAsync();

        return player is null
            ? Results.NotFound()
            : Results.Ok(player);
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

    private static async Task<IResult> Update(
        int id,
        UpdatePlayerRequest request,
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

        var player = await db.Players.FindAsync(id);

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