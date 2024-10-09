#include "tileset.hpp"

void TileSet::initializeTileTextureMap(int tiles_per_row, int tiles_per_col) {
    float tile_size_x = 1.0f / tiles_per_row;  // width of each tile in UV space
    float tile_size_y = 1.0f / tiles_per_col;  // height of each tile in UV space

    // tile IDs start from 0 and go up
    for (int y = 0; y < tiles_per_col; ++y) {
        for (int x = 0; x < tiles_per_row; ++x) {
            int tile_id = y * tiles_per_row + x;

            TileData data;
            data.top_left = glm::vec2(x * tile_size_x, y * tile_size_y);
            data.bottom_right = glm::vec2((x + 1) * tile_size_x, (y + 1) * tile_size_y);

            tile_textures[tile_id] = data;  // texture coordinates for this tile
        }
    }
}
const TileData& TileSet::getTileData(int tile_id) const {
    auto it = tile_textures.find(tile_id);
    if (it != tile_textures.end()) {
        return it->second;  // return the found TileData
    }
    else {
        return it->second;
        // throw error
    }
}
