#pragma once
#include "common.hpp"
#include "components.hpp"  
#include <unordered_map>

const int map_width = 17;
const int map_height = 13;

// Struct to represent a tile set, including its texture mappings
struct TileSet {
    std::unordered_map<int, TEXTURE_ASSET_ID> tile_texture_map;  // Maps tile IDs to their texture IDs

    void initializeTileTextureMap();
};