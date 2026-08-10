using Microsoft.EntityFrameworkCore;
using MultiplayerBackend.Api.Data;
using MultiplayerBackend.Api.Models;

namespace MultiplayerBackend.Api.Services;

public enum TransferItemResult
{
    Success,
    InvalidAmount,
    SamePlayer,
    SourceItemNotFound,
    NotEnoughItems,
    TargetPlayerNotFound,
    ConcurrencyConflict
}

public class InventoryService
{
    private readonly AppDbContext _db;

    public InventoryService(AppDbContext db)
    {
        _db = db;
    }

    public async Task<TransferItemResult> TransferItemAsync(
        int playerId,
        int itemId,
        int targetPlayerId,
        int amount)
    {
        if (amount <= 0)
        {
            return TransferItemResult.InvalidAmount;
        }

        if (playerId == targetPlayerId)
        {
            return TransferItemResult.SamePlayer;
        }

        var sourceItem = await _db.InventoryItems
            .FirstOrDefaultAsync(item =>
                item.Id == itemId &&
                item.PlayerId == playerId);

        if (sourceItem is null)
        {
            return TransferItemResult.SourceItemNotFound;
        }

        if (sourceItem.Amount < amount)
        {
            return TransferItemResult.NotEnoughItems;
        }

        var targetPlayerExists = await _db.Players
            .AnyAsync(player =>
                player.Id == targetPlayerId);

        if (!targetPlayerExists)
        {
            return TransferItemResult.TargetPlayerNotFound;
        }

        var targetItem = await _db.InventoryItems
            .FirstOrDefaultAsync(item =>
                item.PlayerId == targetPlayerId &&
                item.ItemName == sourceItem.ItemName);

        sourceItem.Amount -= amount;

        if (sourceItem.Amount == 0)
        {
            _db.InventoryItems.Remove(sourceItem);
        }

        if (targetItem is null)
        {
            targetItem = new InventoryItem
            {
                PlayerId = targetPlayerId,
                ItemName = sourceItem.ItemName,
                Amount = amount
            };

            _db.InventoryItems.Add(targetItem);
        }
        else
        {
            targetItem.Amount += amount;
        }

        try
        {
            await _db.SaveChangesAsync();
        }
        catch (DbUpdateConcurrencyException)
        {
            return TransferItemResult.ConcurrencyConflict;
        }

        return TransferItemResult.Success;
    }
}