namespace MultiplayerBackend.Api.Models;

public class InventoryItem
{
    public int Id { get; set; }

    public int PlayerId { get; set; }

    public required string ItemName { get; set; }

    public int Amount { get; set; }

    public Player Player { get; set; } = null!;

    public uint Version { get; set; }
}