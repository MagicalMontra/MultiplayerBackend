using System.Net;
using System.Net.Http.Headers;
using System.Net.Http.Json;
using MultiplayerBackend.Api.DTOs;
using MultiplayerBackend.Api.Tests.Infrastructure;

namespace MultiplayerBackend.Api.Tests;

[Collection("ApiIntegration")]
public class InventoryTransferTests
{
    private readonly ApiIntegrationTestFixture _fixture;

    public InventoryTransferTests(
        ApiIntegrationTestFixture fixture)
    {
        _fixture = fixture;
    }

    [Fact]
    public async Task TransferItem_MovesItemsBetweenAuthenticatedPlayers()
    {
        await _fixture.ResetAsync();

        using var client = _fixture.CreateClient();
        
        // -------------------------------------------------------------------------
        // Arrange: register Sett
        // -------------------------------------------------------------------------
    
        var settRegisterResponse = await client!.PostAsJsonAsync(
            "/api/auth/register",
            new RegisterRequest(
                "sett",
                "test-password-123",
                "Sett"));
    
        settRegisterResponse.EnsureSuccessStatusCode();
    
        var settRegistration =
            await settRegisterResponse.Content
                .ReadFromJsonAsync<RegisterResponse>();
    
        Assert.NotNull(settRegistration);
    
        // -------------------------------------------------------------------------
        // Arrange: register Alice
        // -------------------------------------------------------------------------
    
        var aliceRegisterResponse = await client.PostAsJsonAsync(
            "/api/auth/register",
            new RegisterRequest(
                "alice",
                "test-password-456",
                "Alice"));
    
        aliceRegisterResponse.EnsureSuccessStatusCode();
    
        var aliceRegistration =
            await aliceRegisterResponse.Content
                .ReadFromJsonAsync<RegisterResponse>();
    
        Assert.NotNull(aliceRegistration);
    
        // -------------------------------------------------------------------------
        // Login as Sett
        // -------------------------------------------------------------------------
    
        var settLoginResponse = await client.PostAsJsonAsync(
            "/api/auth/login",
            new LoginRequest(
                "sett",
                "test-password-123"));
    
        settLoginResponse.EnsureSuccessStatusCode();
    
        var settLogin =
            await settLoginResponse.Content
                .ReadFromJsonAsync<LoginResponse>();
    
        Assert.NotNull(settLogin);
        Assert.False(string.IsNullOrWhiteSpace(settLogin.AccessToken));
    
        client.DefaultRequestHeaders.Authorization =
            new AuthenticationHeaderValue(
                "Bearer",
                settLogin.AccessToken);
    
        // -------------------------------------------------------------------------
        // Give Sett 10 Health Potions
        // -------------------------------------------------------------------------
    
        var itemResponse = await client.PostAsJsonAsync(
            "/api/players/me/inventory",
            new AddInventoryItemRequest(
                "Health Potion",
                10));
    
        itemResponse.EnsureSuccessStatusCode();
    
        var item =
            await itemResponse.Content
                .ReadFromJsonAsync<InventoryItemResponse>();
    
        Assert.NotNull(item);
    
        // -------------------------------------------------------------------------
        // Act: transfer 3 to Alice
        // -------------------------------------------------------------------------
    
        var transferResponse = await client.PostAsJsonAsync(
            $"/api/players/me/inventory/{item.Id}/transfer",
            new TransferItemRequest(
                aliceRegistration.PlayerId,
                3));
    
        // -------------------------------------------------------------------------
        // Assert transfer succeeded
        // -------------------------------------------------------------------------
    
        Assert.Equal(
            HttpStatusCode.NoContent,
            transferResponse.StatusCode);
    
        // Verify Sett now has 7
        var settInventory =
            await client.GetFromJsonAsync<List<InventoryItemResponse>>(
                "/api/players/me/inventory");
    
        Assert.NotNull(settInventory);
        Assert.Single(settInventory);
        Assert.Equal(7, settInventory[0].Amount);
    
        // -------------------------------------------------------------------------
        // Login as Alice
        // -------------------------------------------------------------------------
    
        var aliceLoginResponse = await client.PostAsJsonAsync(
            "/api/auth/login",
            new LoginRequest(
                "alice",
                "test-password-456"));
    
        aliceLoginResponse.EnsureSuccessStatusCode();
    
        var aliceLogin =
            await aliceLoginResponse.Content
                .ReadFromJsonAsync<LoginResponse>();
    
        Assert.NotNull(aliceLogin);
    
        client.DefaultRequestHeaders.Authorization =
            new AuthenticationHeaderValue(
                "Bearer",
                aliceLogin.AccessToken);
    
        // Verify Alice received 3
        var aliceInventory =
            await client.GetFromJsonAsync<List<InventoryItemResponse>>(
                "/api/players/me/inventory");
    
        Assert.NotNull(aliceInventory);
        Assert.Single(aliceInventory);
        Assert.Equal(3, aliceInventory[0].Amount);
    }
    
    [Fact]
    public async Task GetInventory_WithoutToken_ReturnsUnauthorized()
    {
        await _fixture.ResetAsync();

        using var client = _fixture.CreateClient();

        var response = await client.GetAsync(
            "/api/players/me/inventory");

        Assert.Equal(
            HttpStatusCode.Unauthorized,
            response.StatusCode);
    }
}