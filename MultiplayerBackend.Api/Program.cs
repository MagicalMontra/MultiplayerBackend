using StackExchange.Redis;
using MultiplayerBackend.Api.Data;
using Microsoft.EntityFrameworkCore;
using MultiplayerBackend.Api.Models;
using Microsoft.AspNetCore.Identity;
using MultiplayerBackend.Api.Health;
using Microsoft.IdentityModel.Tokens;
using MultiplayerBackend.Api.Services;
using MultiplayerBackend.Api.Endpoints;
using Microsoft.AspNetCore.Diagnostics.HealthChecks;
using Microsoft.AspNetCore.Authentication.JwtBearer;

var builder = WebApplication.CreateBuilder(args);

builder.Services.AddDbContext<AppDbContext>(options =>
    options.UseNpgsql(
        builder.Configuration.GetConnectionString("DefaultConnection")));

builder.Services.AddScoped<InventoryService>();

builder.Services.AddScoped<
    IPasswordHasher<Account>,
    PasswordHasher<Account>>();

builder.Services.AddScoped<AuthService>();

builder.Services.AddSingleton<TokenService>();

builder.Services.AddOpenApi();

var jwtIssuer = builder.Configuration["Jwt:Issuer"]
                ?? throw new InvalidOperationException(
                    "JWT issuer is not configured.");

var jwtAudience = builder.Configuration["Jwt:Audience"]
                  ?? throw new InvalidOperationException(
                      "JWT audience is not configured.");

var jwtSigningKey = builder.Configuration["Jwt:SigningKey"]
                    ?? throw new InvalidOperationException(
                        "JWT signing key is not configured.");

builder.Services
    .AddAuthentication(JwtBearerDefaults.AuthenticationScheme)
    .AddJwtBearer(options =>
    {
        options.TokenValidationParameters =
            new TokenValidationParameters
            {
                ValidateIssuer = true,
                ValidIssuer = jwtIssuer,

                ValidateAudience = true,
                ValidAudience = jwtAudience,

                ValidateLifetime = true,

                ValidateIssuerSigningKey = true,
                IssuerSigningKey =
                    new SymmetricSecurityKey(
                        Convert.FromBase64String(jwtSigningKey))
            };
    });

builder.Services.AddAuthorization();

builder.Services.AddStackExchangeRedisCache(options =>
{
    options.Configuration =
        builder.Configuration.GetConnectionString("Redis");

    options.InstanceName = "MultiplayerBackend:";
});

var redisConnectionString =
    builder.Configuration.GetConnectionString("Redis")
    ?? throw new InvalidOperationException(
        "Redis connection string is not configured.");

builder.Services.AddSingleton<IConnectionMultiplexer>(_ =>
{
    var options =
        ConfigurationOptions.Parse(redisConnectionString);

    options.AbortOnConnectFail = false;

    return ConnectionMultiplexer.Connect(options);
});

builder.Services.AddSingleton<LoginQueueService>();

builder.Services
    .AddHealthChecks()
    .AddDbContextCheck<AppDbContext>(
        name: "postgresql",
        tags: ["ready"])
    .AddCheck<RedisHealthCheck>(
        name: "redis",
        tags: ["ready"]);

var app = builder.Build();

if (app.Environment.IsDevelopment())
{
    using var scope = app.Services.CreateScope();

    var db = scope.ServiceProvider
        .GetRequiredService<AppDbContext>();

    await db.Database.MigrateAsync();
}

app.UseAuthentication();
app.UseAuthorization();

if (app.Environment.IsDevelopment())
{
    app.MapOpenApi();
}

app.MapPlayerEndpoints();
app.MapInventoryEndpoints();
app.MapAuthEndpoints();
app.MapLoginQueueEndpoints();

app.MapHealthChecks(
    "/health/live",
    new HealthCheckOptions
    {
        Predicate = _ => false
    });

app.MapHealthChecks(
    "/health/ready",
    new HealthCheckOptions
    {
        Predicate = check =>
            check.Tags.Contains("ready")
    });

app.Run();

public partial class Program
{
    
}