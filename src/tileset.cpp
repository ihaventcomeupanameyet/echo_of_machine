#include "tileset.hpp"

// Function to initialize the grassland tileset
void initialize_grassland_tileset(TileSet& tileset, int tiles_per_row) {
    float tile_size_in_atlas = 1.0f / tiles_per_row;  // Assuming tiles_per_row x tiles_per_row grid

    // Example of filling the TileSet with texture coordinates for tile IDs
    tileset.tile_textures[1] = {
        vec2(0.0f, 0.0f),  // Top-left corner of the first tile
        vec2(tile_size_in_atlas, tile_size_in_atlas)  // Bottom-right corner of the first tile
    };

    tileset.tile_textures[2] = {
        vec2(tile_size_in_atlas, 0.0f),  // Top-left corner of the second tile
        vec2(tile_size_in_atlas * 2, tile_size_in_atlas)  // Bottom-right corner of the second tile
    };

    // Add more tiles similarly
    tileset.tile_textures[3] = {
        vec2(tile_size_in_atlas * 2, 0.0f),
        vec2(tile_size_in_atlas * 3, tile_size_in_atlas)
    };

    // Continue adding as many tiles as you need based on the tile IDs
}
