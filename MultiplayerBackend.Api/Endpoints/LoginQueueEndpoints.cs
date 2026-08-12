using System.Security.Claims;
using MultiplayerBackend.Api.DTOs;
using MultiplayerBackend.Api.Extensions;
using MultiplayerBackend.Api.Services;

namespace MultiplayerBackend.Api.Endpoints;

public static class LoginQueueEndpoints
{
    public static void MapLoginQueueEndpoints(
        this WebApplication app)
    {
        var group = app
            .MapGroup("/api/login-queue")
            .RequireAuthorization();

        group.MapPost("/join", Join);
        group.MapGet("/status", GetStatus);
        group.MapDelete("/leave", Leave);
    }

    private static async Task<IResult> Join(
        ClaimsPrincipal user,
        LoginQueueService queue)
    {
        var accountId = user.GetAccountId();

        if (accountId is null)
        {
            return Results.Unauthorized();
        }

        var position =
            await queue.JoinAsync(accountId.Value);

        var count =
            await queue.GetCountAsync();

        return Results.Ok(
            new LoginQueueStatusResponse(
                true,
                position,
                count));
    }

    private static async Task<IResult> GetStatus(
        ClaimsPrincipal user,
        LoginQueueService queue)
    {
        var accountId = user.GetAccountId();

        if (accountId is null)
        {
            return Results.Unauthorized();
        }

        var position =
            await queue.GetPositionAsync(
                accountId.Value);

        var count =
            await queue.GetCountAsync();

        return Results.Ok(
            new LoginQueueStatusResponse(
                position is not null,
                position,
                count));
    }

    private static async Task<IResult> Leave(
        ClaimsPrincipal user,
        LoginQueueService queue)
    {
        var accountId = user.GetAccountId();

        if (accountId is null)
        {
            return Results.Unauthorized();
        }

        await queue.LeaveAsync(
            accountId.Value);

        return Results.NoContent();
    }
}