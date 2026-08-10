using MultiplayerBackend.Api.Data;
using MultiplayerBackend.Api.DTOs;
using MultiplayerBackend.Api.Models;
using Microsoft.EntityFrameworkCore;
using MultiplayerBackend.Api.Services;

namespace MultiplayerBackend.Api.Endpoints;

public static class InventoryEndpoints
{
    public static void MapInventoryEndpoints(this WebApplication app)
    {
        var group = app.MapGroup("/api/players/{playerId:int}/inventory");

        group.MapGet("/", GetInventory);
        group.MapPost("/", AddItem);
        group.MapPost("/{itemId:int}/transfer", TransferItem);
    }

    private static async Task<IResult> GetInventory(
        int playerId,
        AppDbContext db)
    {
        var playerExists = await db.Players
            .AnyAsync(player => player.Id == playerId);

        if (!playerExists)
        {
            return Results.NotFound("Player not found.");
        }

        var items = await db.InventoryItems
            .Where(item => item.PlayerId == playerId)
            .Select(item => new InventoryItemResponse(
                item.Id,
                item.PlayerId,
                item.ItemName,
                item.Amount))
            .ToListAsync();

        return Results.Ok(items);
    }

    private static async Task<IResult> AddItem(
        int playerId,
        AddInventoryItemRequest request,
        AppDbContext db)
    {
        if (string.IsNullOrWhiteSpace(request.ItemName))
        {
            return Results.BadRequest("Item name is required.");
        }

        if (request.Amount <= 0)
        {
            return Results.BadRequest(
                "Amount must be greater than zero.");
        }

        var playerExists = await db.Players
            .AnyAsync(player => player.Id == playerId);

        if (!playerExists)
        {
            return Results.NotFound("Player not found.");
        }

        var item = await db.InventoryItems
            .FirstOrDefaultAsync(item =>
                item.PlayerId == playerId &&
                item.ItemName == request.ItemName);

        if (item is null)
        {
            item = new InventoryItem
            {
                PlayerId = playerId,
                ItemName = request.ItemName,
                Amount = request.Amount
            };

            db.InventoryItems.Add(item);
        }
        else
        {
            item.Amount += request.Amount;
        }

        await db.SaveChangesAsync();

        return Results.Ok(new InventoryItemResponse(
            item.Id,
            item.PlayerId,
            item.ItemName,
            item.Amount));
    }

    private static async Task<IResult> TransferItem(
        int playerId,
        int itemId,
        TransferItemRequest request,
        InventoryService inventoryService)
    {
        var result = await inventoryService.TransferItemAsync(
            playerId,
            itemId,
            request.TargetPlayerId,
            request.Amount);

        return result switch
        {
            TransferItemResult.Success =>
                Results.NoContent(),

            TransferItemResult.InvalidAmount =>
                Results.BadRequest(
                    "Amount must be greater than zero."),

            TransferItemResult.SamePlayer =>
                Results.BadRequest(
                    "Cannot transfer an item to the same player."),

            TransferItemResult.SourceItemNotFound =>
                Results.NotFound(
                    "Source item not found."),

            TransferItemResult.NotEnoughItems =>
                Results.BadRequest(
                    "Not enough items."),

            TransferItemResult.TargetPlayerNotFound =>
                Results.NotFound(
                    "Target player not found."),

            TransferItemResult.ConcurrencyConflict =>
                Results.Conflict(
                    "Inventory changed during the transfer. Please retry."),

            _ => Results.StatusCode(500)
        };
    }
}