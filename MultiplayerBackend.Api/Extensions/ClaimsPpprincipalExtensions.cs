using System.Security.Claims;

namespace MultiplayerBackend.Api.Extensions;

public static class ClaimsPrincipalExtensions
{
    public static int? GetPlayerId(this ClaimsPrincipal user)
    {
        var value = user.FindFirstValue("playerId");

        return int.TryParse(value, out var playerId)
            ? playerId
            : null;
    }
}