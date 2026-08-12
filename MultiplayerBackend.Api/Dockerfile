FROM mcr.microsoft.com/dotnet/sdk:10.0 AS build
WORKDIR /src

COPY MultiplayerBackend.Api/MultiplayerBackend.Api.csproj MultiplayerBackend.Api/
RUN dotnet restore MultiplayerBackend.Api/MultiplayerBackend.Api.csproj

COPY MultiplayerBackend.Api/ MultiplayerBackend.Api/

WORKDIR /src/MultiplayerBackend.Api

RUN dotnet publish \
    -c Release \
    -o /app/publish \
    --no-restore

FROM mcr.microsoft.com/dotnet/aspnet:10.0 AS final
WORKDIR /app

COPY --from=build /app/publish .

EXPOSE 8080

ENTRYPOINT ["dotnet", "MultiplayerBackend.Api.dll"]