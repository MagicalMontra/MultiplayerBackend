# MultiplayerBackend

A multiplayer backend built with ASP.NET Core, PostgreSQL, Redis, JWT authentication, and Docker Compose.

## Architecture

Client
  |
  v
ASP.NET Core API
  |
  +-- PostgreSQL
  |    +-- Accounts
  |    +-- Players
  |    +-- Inventory
  |
  +-- Redis
       +-- Player cache
       +-- Login queue

## Features

- JWT authentication
- Player-owned `/me` endpoints
- Inventory stacking and transfers
- Optimistic concurrency using PostgreSQL `xmin`
- Redis cache-aside caching
- Atomic Redis login queue using Lua
- Liveness and readiness health checks
- EF Core migrations
- Integration tests with Testcontainers
- Docker Compose development environment

## Run locally

Create `.env`:

```env
JWT_SIGNING_KEY=<base64-key>
POSTGRES_PASSWORD=devpassword