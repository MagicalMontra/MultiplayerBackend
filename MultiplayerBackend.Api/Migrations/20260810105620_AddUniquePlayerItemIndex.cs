using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace MultiplayerBackend.Api.Migrations
{
    /// <inheritdoc />
    public partial class AddUniquePlayerItemIndex : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropIndex(
                name: "IX_InventoryItems_PlayerId",
                table: "InventoryItems");

            migrationBuilder.CreateIndex(
                name: "IX_InventoryItems_PlayerId_ItemName",
                table: "InventoryItems",
                columns: new[] { "PlayerId", "ItemName" },
                unique: true);
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropIndex(
                name: "IX_InventoryItems_PlayerId_ItemName",
                table: "InventoryItems");

            migrationBuilder.CreateIndex(
                name: "IX_InventoryItems_PlayerId",
                table: "InventoryItems",
                column: "PlayerId");
        }
    }
}
