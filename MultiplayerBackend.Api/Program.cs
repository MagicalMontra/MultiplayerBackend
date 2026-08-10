using MultiplayerBackend.Api.Data;
using Microsoft.EntityFrameworkCore;
using MultiplayerBackend.Api.Models;
using Microsoft.AspNetCore.Identity;
using MultiplayerBackend.Api.Services;
using MultiplayerBackend.Api.Endpoints;

var builder = WebApplication.CreateBuilder(args);

builder.Services.AddDbContext<AppDbContext>(options =>
    options.UseNpgsql(
        builder.Configuration.GetConnectionString("DefaultConnection")));

builder.Services.AddScoped<InventoryService>();

builder.Services.AddScoped<
    IPasswordHasher<Account>,
    PasswordHasher<Account>>();

builder.Services.AddScoped<AuthService>();

builder.Services.AddOpenApi();

var app = builder.Build();

if (app.Environment.IsDevelopment())
{
    app.MapOpenApi();
}

app.MapPlayerEndpoints();
app.MapInventoryEndpoints();
app.MapAuthEndpoints();

app.Run();

public partial class Program
{
    
}