namespace MultiplayerBackend.Api.Models;

public class Account
{
    public int Id { get; set; }

    public required string Username { get; set; }

    public required string PasswordHash { get; set; }

    public int PlayerId { get; set; }

    public Player Player { get; set; } = null!;
}