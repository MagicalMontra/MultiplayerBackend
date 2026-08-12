using Microsoft.Extensions.Diagnostics.HealthChecks;
using StackExchange.Redis;

namespace MultiplayerBackend.Api.Health;

public class RedisHealthCheck : IHealthCheck
{
    private readonly IConnectionMultiplexer _redis;

    public RedisHealthCheck(
        IConnectionMultiplexer redis)
    {
        _redis = redis;
    }

    public async Task<HealthCheckResult> CheckHealthAsync(
        HealthCheckContext context,
        CancellationToken cancellationToken = default)
    {
        try
        {
            var db = _redis.GetDatabase();

            var latency = await db
                .PingAsync()
                .WaitAsync(
                    TimeSpan.FromSeconds(1),
                    cancellationToken);

            return HealthCheckResult.Healthy(
                $"Redis responded in {latency.TotalMilliseconds:F1} ms.");
        }
        catch
        {
            return HealthCheckResult.Unhealthy(
                "Redis is unavailable.");
        }
    }
}