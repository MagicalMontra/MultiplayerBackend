using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace MultiplayerBackend.Api.Migrations
{
    /// <inheritdoc />
    public partial class AddInventoryConcurrency : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.AddColumn<uint>(
                name: "xmin",
                table: "InventoryItems",
                type: "xid",
                rowVersion: true,
                nullable: false,
                defaultValue: 0u);
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropColumn(
                name: "xmin",
                table: "InventoryItems");
        }
    }
}
