using MultiplayerBackend.Api.Models;

public class Player
{
    public int Id { get; set; }

    public required string Name { get; set; }

    public int Level { get; set; }

    public List<InventoryItem> Inventory { get; set; } = [];
}