#include "tileset.hpp"

// initialize  tile texture map with mappings for tile IDs
void TileSet::initializeTileTextureMap() {
    tile_texture_map[0] = TEXTURE_ASSET_ID::TILE_0;   
    tile_texture_map[1] = TEXTURE_ASSET_ID::TILE_1;
    tile_texture_map[2] = TEXTURE_ASSET_ID::TILE_2; 
    tile_texture_map[3] = TEXTURE_ASSET_ID::TILE_3; 
}

