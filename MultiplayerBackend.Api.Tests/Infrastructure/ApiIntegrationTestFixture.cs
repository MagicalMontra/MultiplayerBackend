using System.Text;
using Microsoft.AspNetCore.Hosting;
using Microsoft.AspNetCore.Mvc.Testing;
using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.Caching.Distributed;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.DependencyInjection.Extensions;
using MultiplayerBackend.Api.Data;
using StackExchange.Redis;
using Testcontainers.PostgreSql;
using Testcontainers.Redis;

namespace MultiplayerBackend.Api.Tests.Infrastructure;

public sealed class ApiIntegrationTestFixture : IAsyncLifetime
{
    private readonly PostgreSqlContainer _postgres =
        new PostgreSqlBuilder("postgres:18")
            .WithDatabase("multiplayer_test")
            .WithUsername("postgres")
            .WithPassword("testpassword")
            .Build();

    private readonly RedisContainer _redis =
        new RedisBuilder("redis:8.8")
            .Build();

    public WebApplicationFactory<Program> Factory { get; private set; } = null!;

    public HttpClient CreateClient()
    {
        return Factory.CreateClient();
    }

    public async Task InitializeAsync()
    {
        // PostgreSQL and Redis are independent, so start them concurrently.
        await Task.WhenAll(
            _postgres.StartAsync(),
            _redis.StartAsync()
        );

        var testSigningKey = Convert.ToBase64String(
            Encoding.UTF8.GetBytes(
                "integration-test-jwt-signing-key-that-is-long-enough-1234567890"));

        var redisConnectionString =
            _redis.GetConnectionString();

        Factory = new WebApplicationFactory<Program>()
            .WithWebHostBuilder(builder =>
            {
                // JWT config used by Program.cs during test startup.
                builder.UseSetting(
                    "Jwt:Issuer",
                    "MultiplayerBackend.Api");

                builder.UseSetting(
                    "Jwt:Audience",
                    "MultiplayerBackend.Client");

                builder.UseSetting(
                    "Jwt:ExpirationMinutes",
                    "60");

                builder.UseSetting(
                    "Jwt:SigningKey",
                    testSigningKey);

                builder.ConfigureServices(services =>
                {
                    // ---------------------------------------------------------
                    // PostgreSQL
                    // ---------------------------------------------------------

                    services.RemoveAll<
                        DbContextOptions<AppDbContext>>();

                    services.AddDbContext<AppDbContext>(options =>
                        options.UseNpgsql(
                            _postgres.GetConnectionString()));

                    // ---------------------------------------------------------
                    // Redis distributed cache
                    // ---------------------------------------------------------

                    services.RemoveAll<IDistributedCache>();

                    services.AddStackExchangeRedisCache(options =>
                    {
                        options.Configuration =
                            redisConnectionString;

                        options.InstanceName =
                            "MultiplayerBackend:";
                    });

                    // ---------------------------------------------------------
                    // Direct StackExchange.Redis connection
                    // ---------------------------------------------------------

                    services.RemoveAll<IConnectionMultiplexer>();

                    services.AddSingleton<IConnectionMultiplexer>(_ =>
                    {
                        var options =
                            ConfigurationOptions.Parse(
                                redisConnectionString);

                        // Only enabled for the disposable test Redis instance.
                        // Lets ResetAsync() call FLUSHDB.
                        options.AllowAdmin = true;

                        return ConnectionMultiplexer.Connect(options);
                    });
                });
            });

        // Force creation of the application and apply migrations once.
        using var scope = Factory.Services.CreateScope();

        var db = scope.ServiceProvider
            .GetRequiredService<AppDbContext>();

        await db.Database.MigrateAsync();
    }

    public async Task ResetAsync()
    {
        // -------------------------------------------------------------
        // Reset PostgreSQL data but KEEP schema/migrations.
        // -------------------------------------------------------------

        using (var scope = Factory.Services.CreateScope())
        {
            var db = scope.ServiceProvider
                .GetRequiredService<AppDbContext>();

            await db.Database.ExecuteSqlRawAsync("""
                TRUNCATE TABLE
                    "Accounts",
                    "InventoryItems",
                    "Players"
                RESTART IDENTITY CASCADE;
                """);
        }

        // -------------------------------------------------------------
        // Reset Redis
        // -------------------------------------------------------------

        var redis = Factory.Services
            .GetRequiredService<IConnectionMultiplexer>();

        var endpoint = redis.GetEndPoints().First();
        var server = redis.GetServer(endpoint);

        await server.FlushDatabaseAsync();
    }

    public async Task DisposeAsync()
    {
        await Factory.DisposeAsync();

        await _redis.DisposeAsync();
        await _postgres.DisposeAsync();
    }
}