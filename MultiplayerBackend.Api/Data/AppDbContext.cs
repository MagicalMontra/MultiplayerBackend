using Microsoft.EntityFrameworkCore;
using MultiplayerBackend.Api.Models;

namespace MultiplayerBackend.Api.Data;

public class AppDbContext : DbContext
{
    public AppDbContext(DbContextOptions<AppDbContext> options)
        : base(options)
    {
    }

    public DbSet<Player> Players => Set<Player>();
    public DbSet<Account> Accounts => Set<Account>();
    public DbSet<InventoryItem> InventoryItems => Set<InventoryItem>();
    
    protected override void OnModelCreating(ModelBuilder modelBuilder)
    {
        modelBuilder.Entity<Player>()
            .HasIndex(player => player.Name);

        modelBuilder.Entity<InventoryItem>()
            .HasIndex(item => new
            {
                item.PlayerId,
                item.ItemName
            })
            .IsUnique();
        
        modelBuilder.Entity<InventoryItem>()
            .Property(item => item.Version)
            .IsRowVersion();
        
        modelBuilder.Entity<Account>()
            .HasIndex(account => account.Username)
            .IsUnique();

        modelBuilder.Entity<Account>()
            .HasIndex(account => account.PlayerId)
            .IsUnique();
    }
}