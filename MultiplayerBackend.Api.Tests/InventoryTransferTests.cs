using System.Net;
using System.Net.Http.Json;
using Microsoft.AspNetCore.Hosting;
using Microsoft.AspNetCore.Mvc.Testing;
using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.DependencyInjection;
using MultiplayerBackend.Api.Data;
using MultiplayerBackend.Api.DTOs;
using Testcontainers.PostgreSql;

namespace MultiplayerBackend.Api.Tests;

public class InventoryTransferTests : IAsyncLifetime
{
    private readonly PostgreSqlContainer _postgres =
        new PostgreSqlBuilder("postgres:18")
            .WithDatabase("multiplayer_test")
            .WithUsername("postgres")
            .WithPassword("testpassword")
            .Build();

    private WebApplicationFactory<Program>? _factory;
    private HttpClient? _client;

    public async Task InitializeAsync()
    {
        await _postgres.StartAsync();

        _factory = new WebApplicationFactory<Program>()
            .WithWebHostBuilder(builder =>
            {
                builder.ConfigureServices(services =>
                {
                    var descriptor = services.SingleOrDefault(
                        service =>
                            service.ServiceType ==
                            typeof(DbContextOptions<AppDbContext>));

                    if (descriptor is not null)
                    {
                        services.Remove(descriptor);
                    }

                    services.AddDbContext<AppDbContext>(options =>
                        options.UseNpgsql(
                            _postgres.GetConnectionString()));
                });
            });

        using var scope = _factory.Services.CreateScope();

        var db = scope.ServiceProvider
            .GetRequiredService<AppDbContext>();

        await db.Database.MigrateAsync();

        _client = _factory.CreateClient();
    }

    public async Task DisposeAsync()
    {
        _client?.Dispose();

        if (_factory is not null)
        {
            await _factory.DisposeAsync();
        }

        await _postgres.DisposeAsync();
    }

    [Fact]
    public async Task TransferItem_MovesItemsBetweenPlayers()
    {
        // Arrange

        var settResponse = await _client!.PostAsJsonAsync(
            "/api/players",
            new CreatePlayerRequest("Sett", 51));

        settResponse.EnsureSuccessStatusCode();

        var sett = await settResponse.Content
            .ReadFromJsonAsync<PlayerResponse>();

        var aliceResponse = await _client.PostAsJsonAsync(
            "/api/players",
            new CreatePlayerRequest("Alice", 20));

        aliceResponse.EnsureSuccessStatusCode();

        var alice = await aliceResponse.Content
            .ReadFromJsonAsync<PlayerResponse>();

        var itemResponse = await _client.PostAsJsonAsync(
            $"/api/players/{sett!.Id}/inventory",
            new AddInventoryItemRequest(
                "Health Potion",
                10));

        itemResponse.EnsureSuccessStatusCode();

        var item = await itemResponse.Content
            .ReadFromJsonAsync<InventoryItemResponse>();

        // Act

        var transferResponse = await _client.PostAsJsonAsync(
            $"/api/players/{sett.Id}/inventory/{item!.Id}/transfer",
            new TransferItemRequest(
                alice!.Id,
                3));

        // Assert

        Assert.Equal(
            HttpStatusCode.NoContent,
            transferResponse.StatusCode);

        var settInventory =
            await _client.GetFromJsonAsync<List<InventoryItemResponse>>(
                $"/api/players/{sett.Id}/inventory");

        var aliceInventory =
            await _client.GetFromJsonAsync<List<InventoryItemResponse>>(
                $"/api/players/{alice.Id}/inventory");

        Assert.Single(settInventory!);
        Assert.Equal(7, settInventory[0].Amount);

        Assert.Single(aliceInventory!);
        Assert.Equal(3, aliceInventory[0].Amount);
    }
}