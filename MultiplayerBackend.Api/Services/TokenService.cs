using System.IdentityModel.Tokens.Jwt;
using System.Security.Claims;
using Microsoft.IdentityModel.Tokens;
using MultiplayerBackend.Api.Models;

namespace MultiplayerBackend.Api.Services;

public record AccessTokenResult(
    string Token,
    DateTime ExpiresAtUtc
);

public class TokenService
{
    private readonly IConfiguration _configuration;

    public TokenService(IConfiguration configuration)
    {
        _configuration = configuration;
    }

    public AccessTokenResult CreateAccessToken(Account account)
    {
        var issuer = _configuration["Jwt:Issuer"]
            ?? throw new InvalidOperationException(
                "JWT issuer is not configured.");

        var audience = _configuration["Jwt:Audience"]
            ?? throw new InvalidOperationException(
                "JWT audience is not configured.");

        var signingKey = _configuration["Jwt:SigningKey"]
            ?? throw new InvalidOperationException(
                "JWT signing key is not configured.");

        var expirationMinutes =
            _configuration.GetValue<int>("Jwt:ExpirationMinutes");

        if (expirationMinutes <= 0)
        {
            throw new InvalidOperationException(
                "JWT expiration must be greater than zero.");
        }

        var now = DateTime.UtcNow;
        var expiresAt = now.AddMinutes(expirationMinutes);

        var claims = new[]
        {
            new Claim(
                JwtRegisteredClaimNames.Sub,
                account.Id.ToString()),

            new Claim(
                "playerId",
                account.PlayerId.ToString()),

            new Claim(
                JwtRegisteredClaimNames.UniqueName,
                account.Username),

            new Claim(
                JwtRegisteredClaimNames.Jti,
                Guid.NewGuid().ToString())
        };

        var keyBytes = Convert.FromBase64String(signingKey);

        var key = new SymmetricSecurityKey(keyBytes);

        var credentials = new SigningCredentials(
            key,
            SecurityAlgorithms.HmacSha256);

        var token = new JwtSecurityToken(
            issuer: issuer,
            audience: audience,
            claims: claims,
            notBefore: now,
            expires: expiresAt,
            signingCredentials: credentials);

        var tokenString =
            new JwtSecurityTokenHandler()
                .WriteToken(token);

        return new AccessTokenResult(
            tokenString,
            expiresAt);
    }
}