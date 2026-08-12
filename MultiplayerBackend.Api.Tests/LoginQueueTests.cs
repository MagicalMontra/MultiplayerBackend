using System.Net;
using System.Net.Http.Headers;
using System.Net.Http.Json;
using MultiplayerBackend.Api.DTOs;
using MultiplayerBackend.Api.Tests.Infrastructure;

namespace MultiplayerBackend.Api.Tests;

[Collection("ApiIntegration")]
public class LoginQueueTests
{
    private readonly ApiIntegrationTestFixture _fixture;

    public LoginQueueTests(
        ApiIntegrationTestFixture fixture)
    {
        _fixture = fixture;
    }

    [Fact]
    public async Task ConcurrentJoin_ForSameAccount_AddsOnlyOnce()
    {
        await _fixture.ResetAsync();

        using var client = _fixture.CreateClient();

        // -------------------------------------------------------------
        // Register
        // -------------------------------------------------------------

        var registerResponse =
            await client.PostAsJsonAsync(
                "/api/auth/register",
                new RegisterRequest(
                    "sett",
                    "test-password-123",
                    "Sett"));

        registerResponse.EnsureSuccessStatusCode();

        // -------------------------------------------------------------
        // Login
        // -------------------------------------------------------------

        var loginResponse =
            await client.PostAsJsonAsync(
                "/api/auth/login",
                new LoginRequest(
                    "sett",
                    "test-password-123"));

        loginResponse.EnsureSuccessStatusCode();

        var login =
            await loginResponse.Content
                .ReadFromJsonAsync<LoginResponse>();

        Assert.NotNull(login);

        client.DefaultRequestHeaders.Authorization =
            new AuthenticationHeaderValue(
                "Bearer",
                login.AccessToken);

        // -------------------------------------------------------------
        // Act: 20 concurrent joins
        // -------------------------------------------------------------

        var tasks = Enumerable.Range(0, 20)
            .Select(_ =>
                client.PostAsync(
                    "/api/login-queue/join",
                    null))
            .ToArray();

        var responses =
            await Task.WhenAll(tasks);

        // -------------------------------------------------------------
        // Assert
        // -------------------------------------------------------------

        foreach (var response in responses)
        {
            Assert.Equal(
                HttpStatusCode.OK,
                response.StatusCode);

            var result =
                await response.Content
                    .ReadFromJsonAsync<
                        LoginQueueStatusResponse>();

            Assert.NotNull(result);

            Assert.Equal(
                1,
                result.Position);
        }

        var status =
            await client.GetFromJsonAsync<
                LoginQueueStatusResponse>(
                "/api/login-queue/status");

        Assert.NotNull(status);

        Assert.True(status.InQueue);
        Assert.Equal(1, status.Position);
        Assert.Equal(1, status.TotalPlayers);
    }
}