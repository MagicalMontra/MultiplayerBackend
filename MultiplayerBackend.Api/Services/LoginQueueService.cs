using StackExchange.Redis;

namespace MultiplayerBackend.Api.Services;

public class LoginQueueService
{
    private const string QueueKey =
        "MultiplayerBackend:login:queue";

    private const string SequenceKey =
        "MultiplayerBackend:login:queue:sequence";

    private readonly IConnectionMultiplexer _redis;

    public LoginQueueService(
        IConnectionMultiplexer redis)
    {
        _redis = redis;
    }

    public async Task<long> JoinAsync(int accountId)
    {
        var db = _redis.GetDatabase();

        var member = $"account:{accountId}";

        const string script = """
                              local rank = redis.call('ZRANK', KEYS[1], ARGV[1])

                              if rank then
                                  return rank + 1
                              end

                              local sequence = redis.call('INCR', KEYS[2])

                              redis.call(
                                  'ZADD',
                                  KEYS[1],
                                  sequence,
                                  ARGV[1]
                              )

                              rank = redis.call(
                                  'ZRANK',
                                  KEYS[1],
                                  ARGV[1]
                              )

                              return rank + 1
                              """;

        var result = await db.ScriptEvaluateAsync(
            script,
            new RedisKey[]
            {
                QueueKey,
                SequenceKey
            },
            new RedisValue[]
            {
                member
            });

        return (long)result;
    }

    public async Task<long?> GetPositionAsync(
        int accountId)
    {
        var db = _redis.GetDatabase();

        var member = $"account:{accountId}";

        var rank =
            await db.SortedSetRankAsync(
                QueueKey,
                member);

        return rank is null
            ? null
            : rank.Value + 1;
    }

    public async Task<long> GetCountAsync()
    {
        var db = _redis.GetDatabase();

        return await db.SortedSetLengthAsync(
            QueueKey);
    }

    public async Task LeaveAsync(int accountId)
    {
        var db = _redis.GetDatabase();

        await db.SortedSetRemoveAsync(
            QueueKey,
            $"account:{accountId}");
    }
}