using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace MultiplayerBackend.Api.Migrations
{
    /// <inheritdoc />
    public partial class AddPlayerNameIndex : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.CreateIndex(
                name: "IX_Players_Name",
                table: "Players",
                column: "Name");
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropIndex(
                name: "IX_Players_Name",
                table: "Players");
        }
    }
}
