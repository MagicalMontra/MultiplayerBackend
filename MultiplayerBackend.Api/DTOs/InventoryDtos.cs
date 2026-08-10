namespace MultiplayerBackend.Api.DTOs;

public record AddInventoryItemRequest(
    string ItemName,
    int Amount
);

public record InventoryItemResponse(
    int Id,
    int PlayerId,
    string ItemName,
    int Amount
);

public record TransferItemRequest(
    int TargetPlayerId,
    int Amount
);