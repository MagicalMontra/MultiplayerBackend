using System.Net.Http.Headers;
using System.Net.Http.Json;
using MultiplayerBackend.Api.DTOs;
using MultiplayerBackend.Api.Tests.Infrastructure;

namespace MultiplayerBackend.Api.Tests;

[Collection("ApiIntegration")]
public class PlayerCacheTests
{
    private readonly ApiIntegrationTestFixture _fixture;

    public PlayerCacheTests(ApiIntegrationTestFixture fixture)
    {
        _fixture = fixture;
    }

    [Fact]
    public async Task UpdateMe_InvalidatesCachedPlayer()
    {
        await _fixture.ResetAsync();

        using var client = _fixture.CreateClient();

        var registerResponse = await client.PostAsJsonAsync(
            "/api/auth/register",
            new RegisterRequest(
                "sett",
                "test-password-123",
                "Sett"));

        registerResponse.EnsureSuccessStatusCode();

        var loginResponse = await client.PostAsJsonAsync(
            "/api/auth/login",
            new LoginRequest(
                "sett",
                "test-password-123"));

        loginResponse.EnsureSuccessStatusCode();

        var login = await loginResponse.Content
            .ReadFromJsonAsync<LoginResponse>();

        Assert.NotNull(login);

        client.DefaultRequestHeaders.Authorization =
            new AuthenticationHeaderValue(
                "Bearer",
                login.AccessToken);

        // Warm the cache with the original player state.
        var cachedPlayer = await client.GetFromJsonAsync<PlayerResponse>(
            "/api/players/me");

        Assert.NotNull(cachedPlayer);
        Assert.Equal("Sett", cachedPlayer.Name);
        Assert.Equal(1, cachedPlayer.Level);

        var updateResponse = await client.PutAsJsonAsync(
            "/api/players/me",
            new UpdatePlayerRequest(
                "Sett Updated",
                10));

        updateResponse.EnsureSuccessStatusCode();

        // The next read must not return the stale cached value.
        var updatedPlayer = await client.GetFromJsonAsync<PlayerResponse>(
            "/api/players/me");

        Assert.NotNull(updatedPlayer);
        Assert.Equal("Sett Updated", updatedPlayer.Name);
        Assert.Equal(10, updatedPlayer.Level);
    }
}
