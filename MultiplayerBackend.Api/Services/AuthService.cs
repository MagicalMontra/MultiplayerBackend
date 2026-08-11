using Microsoft.AspNetCore.Identity;
using Microsoft.EntityFrameworkCore;
using MultiplayerBackend.Api.Data;
using MultiplayerBackend.Api.DTOs;
using MultiplayerBackend.Api.Models;

namespace MultiplayerBackend.Api.Services;

public class AuthService
{
    private readonly AppDbContext _db;
    private readonly IPasswordHasher<Account> _passwordHasher;
    private readonly TokenService _tokenService;

    public AuthService(
        AppDbContext db,
        IPasswordHasher<Account> passwordHasher,
        TokenService tokenService)
    {
        _db = db;
        _passwordHasher = passwordHasher;
        _tokenService = tokenService;
    }

    public async Task<RegisterResponse?> RegisterAsync(
        RegisterRequest request)
    {
        var usernameExists = await _db.Accounts
            .AnyAsync(account =>
                account.Username == request.Username);

        if (usernameExists)
        {
            return null;
        }

        var player = new Player
        {
            Name = request.PlayerName,
            Level = 1
        };

        var account = new Account
        {
            Username = request.Username,
            PasswordHash = string.Empty,
            Player = player
        };

        account.PasswordHash =
            _passwordHasher.HashPassword(
                account,
                request.Password);

        _db.Accounts.Add(account);

        await _db.SaveChangesAsync();

        return new RegisterResponse(
            account.Id,
            player.Id,
            account.Username,
            player.Name);
    }
    
    public async Task<LoginResponse?> LoginAsync(
        LoginRequest request)
    {
        var account = await _db.Accounts
            .FirstOrDefaultAsync(account =>
                account.Username == request.Username);

        if (account is null)
        {
            return null;
        }

        var verificationResult =
            _passwordHasher.VerifyHashedPassword(
                account,
                account.PasswordHash,
                request.Password);

        if (verificationResult ==
            PasswordVerificationResult.Failed)
        {
            return null;
        }

        var accessToken =
            _tokenService.CreateAccessToken(account);

        return new LoginResponse(
            account.Id,
            account.PlayerId,
            account.Username,
            accessToken.Token,
            accessToken.ExpiresAtUtc);
    }
}