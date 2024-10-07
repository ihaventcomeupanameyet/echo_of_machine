#pragma once
#include "common.hpp"
#include <unordered_map>

const int map_width = 10;
const int map_height = 10;
// This struct holds the texture coordinates for a single tile within a tile atlas
struct TileData {
    vec2 top_left;     // Top-left corner of the tile in the atlas
    vec2 bottom_right; // Bottom-right corner of the tile in the atlas
};

// This struct represents a tileset that maps tile IDs to texture coordinates within a tile atlas
struct TileSet {
    std::unordered_map<int, TileData> tile_textures;  // Maps tile IDs to their texture coordinates
};

// Function to initialize the TileSet with texture coordinates
void initialize_grassland_tileset(TileSet& tileset, int tiles_per_row);
