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


std::vector<std::vector<int>> TileSet::initializeGrassMap() {
    std::vector<std::vector<int>> grass_map = {
    {19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19},
    {19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,  8,  9},
    {19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,  8,  9},
    {19, 19, 19, 19, 19, 19,  0, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,  3, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,  8,  9},
    {19, 19, 19, 19, 19, 19,  7, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 10, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,  8,  9},
    {19, 19, 19, 19, 19, 19,  7, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 10, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19},
    { 8,  9, 19, 19, 19, 19, 14, 15, 15, 15, 15, 15, 15, 16, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 17, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19},
    { 8,  9, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19},
    { 8,  9, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19},
    { 8,  9, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19},
    {19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19},
    {19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19},
    {19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19},
    {19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19},
    {19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19},
    {19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19},
    {19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19},
    {19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19},
    {19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19},
    {19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19},
    {19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19},
    {19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19},
    {19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19},
    {19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19},
    {19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19},
    {19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19},
    {19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19},
    {19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19},
    {19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19},
    {19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19}
    };
    return grass_map;
}


// Function to initialize the obstacle map
std::vector<std::vector<int>> TileSet::initializeObstacleMap() {
    std::vector<std::vector<int>> obstacle_map = {
     {12, 12, 13, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 11, 12, 12, 12, 13, 21, 21, 21, 21, 21, 21},
    {12, 12, 13,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11, 12, 12, 12, 13,  0,  0,  0,  0,  0,  0},
    {12, 12, 13,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11, 12, 12, 12, 13,  0,  0,  0,  0,  0,  0},
    {12, 12, 13,  0,  0,  0,  0,  4,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  6,  0,  0,  0, 11, 12, 12, 12, 13,  0,  0,  0,  0,  0,  0},
    {12, 12, 13,  0,  0,  0,  0, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13,  0,  0,  0, 11, 12, 12, 12, 13,  0,  0,  0,  0,  0,  0},
    {22, 21, 21,  0,  0,  0,  0, 21, 28, 21, 21, 21, 21, 23, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,  0,  0,  0, 11, 12, 12, 12, 13,  0,  0,  0,  0,  4,  5},
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11, 12, 12, 12, 13,  0,  0,  0,  0, 11, 12},
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 21, 21, 21, 11, 13,  0,  0,  0,  0, 11, 12},
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11, 13,  0,  0,  0,  0, 11, 12},
    { 0,  0,  0,  0,  0,  0,  0,  4,  5,  5,  5,  5,  5,  5,  5,  6,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11, 13,  0,  0,  0,  0, 11, 12},
    { 5,  5,  6,  0,  0,  0,  0, 11, 12, 12, 12, 12, 12, 12, 12, 13,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,  6,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11, 13,  0,  0,  0,  0, 11, 12},
    {12, 12, 13,  0,  0,  0,  0, 11, 12, 12, 12, 12, 12, 12, 12, 13,  0,  0,  0,  4,  5,  5,  5,  5,  6,  0,  0,  0,  0,  0,  0, 11, 13,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11, 13,  0,  0,  0,  0, 11, 12},
    {12, 12, 13,  0,  0,  0,  0, 11, 12, 12, 12, 12, 12, 12, 12, 13,  0,  0,  0, 11, 12, 12, 12, 12, 13,  0,  0,  0,  0,  0,  0, 11, 13,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11, 13,  0,  0,  0,  0, 11, 12},
    {12, 12, 13,  0,  0,  0,  0, 11, 12, 12, 12, 13, 22, 22, 22, 22,  0,  0,  0, 11, 12, 12, 12, 12, 13,  0,  0,  0,  4,  5,  5, 12, 12,  5,  5,  6,  0,  0,  0,  0,  0,  0, 11, 13,  0,  0,  0,  0, 11, 12},
    {12, 12, 13,  0,  0,  0,  0, 11, 12, 12, 12, 13,  0,  0,  0,  0,  0,  0,  0, 11, 12, 12, 12, 12, 13,  0,  0,  0, 11, 12, 12, 12, 12, 12, 12, 13,  0,  0,  0,  0,  0,  0, 11, 13,  0,  0,  0,  0, 11, 12},
    {12, 12, 13,  0,  0,  0,  0, 11, 12, 12, 12, 13,  0,  0,  0,  0,  0,  0,  0, 11, 12, 12, 12, 12, 13,  0,  0,  0, 28, 28, 28, 11, 13, 28, 28, 28,  0,  0,  0,  4,  5,  5, 12, 13,  0,  0,  0,  0, 11, 12},
    {12, 12, 13,  0,  0,  0,  0, 11, 12, 12, 12, 13,  0,  0,  0,  0,  0,  0,  0, 11, 12, 12, 12, 12, 13,  0,  0,  0,  0,  0,  0, 11, 13,  0,  0,  0,  0,  0,  0, 11, 12, 12, 12, 13,  0,  0,  0,  0, 11, 12},
    {12, 12, 13,  0,  0,  0,  0, 22, 22, 22, 22, 22,  0,  0,  0,  0,  0,  0,  0, 11, 12, 12, 12, 12, 13,  0,  0,  0,  0,  0,  0, 11, 13,  0,  0,  0,  0,  0,  0, 21, 21, 21, 21, 21,  0,  0,  0,  0, 11, 12},
    {12, 12, 13,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11, 12, 12, 12, 12, 13,  0,  0,  0,  0,  0,  0, 28, 28,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11, 12},
    {12, 12, 13,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11, 12, 12, 12, 12, 13,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11, 12},
    {12, 12, 13,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11, 12, 12, 12, 12, 13,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11, 12},
    {12, 12, 13,  0,  0,  0,  0,  4,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5, 12, 12, 12, 12, 12, 13,  0,  0,  0,  0,  0,  0,  4,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  6,  0,  0,  0,  0, 11, 12},
    {12, 12, 13,  0,  0,  0,  0, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13,  0,  0,  0,  0,  0,  0, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13,  0,  0,  0,  0, 11, 12},
    {12, 12, 13,  0,  0,  0,  0, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13,  0,  0,  0,  0,  0,  0, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13,  0,  0,  0,  0, 11, 12},
    {12, 12, 13,  0,  0,  0,  0, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13,  0,  0,  0,  0,  0,  0, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13,  0,  0,  0,  0, 11, 12},
    {12, 12, 13,  0,  0,  0,  0, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 11, 12, 12, 13,  0,  0,  0,  0,  0,  0, 11, 12, 13, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,  0,  0,  0,  0, 11, 12},
    {12, 12, 13,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11, 12, 12, 13,  0,  0,  0,  0,  0,  0, 11, 12, 13,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11, 12},
    {12, 12, 13,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11, 12, 12, 13,  0,  0,  0,  0,  0,  0, 11, 12, 13,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11, 12},
    {12, 12, 13,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11, 12, 12, 13,  0,  0,  0,  0,  0,  0, 11, 12, 13,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11, 12},
    {12, 12, 12,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5, 12, 12, 12, 12,  5,  5,  5,  5,  5,  5, 12, 12, 12,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5, 12, 12},
    };
    return obstacle_map;
}


std::vector<std::vector<int>> TileSet::initializeNewGrassMap() {
    std::vector<std::vector<int>> grass_map = {
    {40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40},
    {39, 39, 39, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40},
    {42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 43, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 38, 40, 40, 40},
    {42, 42, 42, 42, 38, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40},
    {39, 39, 39, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40},
    {40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40},
    {40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40},
    {40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40},
    {40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 44, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 38, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40},
    {40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40},
    {40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40},
    {40, 40, 40, 40, 36, 40, 37, 42, 42, 42, 42, 42, 42, 42, 42, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40},
    {40, 40, 40, 40, 36, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40},
    {40, 40, 40, 40, 36, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40},
    {40, 40, 40, 40, 36, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40},
    {40, 40, 40, 40, 36, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40},
    {40, 40, 40, 40, 36, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40},
    {40, 40, 40, 40, 36, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40},
    {40, 40, 40, 40, 36, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40},
    {40, 40, 40, 40, 36, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40},
    {40, 40, 40, 40, 36, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40},
    {40, 40, 40, 40, 44, 42, 43, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 43, 42, 42, 42, 42, 42, 42, 42, 45, 40, 40, 40},
    {40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40},
    {40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40},
    {40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40},
    {40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40},
    {40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40},
    {40, 40, 40, 40, 40, 40, 44, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 38, 37, 42, 42, 42, 45, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40},
    {40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 36, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40},
    {40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 36, 36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40}
    };
    return grass_map;
}


// Function to initialize the obstacle map
std::vector<std::vector<int>> TileSet::initializeNewObstacleMap() {
    std::vector<std::vector<int>> obstacle_map = {
    {29, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 27, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 31, 49, 50},
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 52, 53},
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 52, 53},
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 52, 53},
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 49, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 51,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 52, 53},
    {50, 50, 51,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 49, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 51,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 52, 53},
    {53, 53, 54,  0,  0,  0,  0,  0,  0,  0, 49, 51,  0,  0,  0, 41, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 55,  0,  0,  0, 49, 51,  0,  0,  0,  0,  0,  0, 52, 53},
    { 0,  0,  0,  0,  0,  0,  0,  0,  0, 49, 53, 54,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 52, 53, 51,  0,  0,  0,  0,  0, 52, 53},
    { 0,  0,  0,  0,  0,  0,  0,  0, 49, 53, 53, 54,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 52, 53, 53, 51,  0,  0,  0,  0, 52, 53},
    { 0,  0,  0,  0,  0,  0,  0,  0, 41, 48, 48, 55,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 41, 48, 48, 55,  0,  0,  0,  0, 52, 53},
    {53, 53, 54,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 49, 50, 24, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 24, 50, 51,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 52, 53},
    {53, 53, 54,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 25, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 54,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 52, 53},
    {53, 53, 54,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 52, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 54,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 52, 53},
    {53, 53, 54,  0,  0,  0,  0,  0, 49, 50, 50, 51,  0,  0,  0, 52, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 54,  0,  0,  0, 49, 50, 50, 51,  0,  0,  0,  0, 52, 53},
    {53, 53, 54,  0,  0,  0,  0,  0, 52, 53, 53, 54,  0,  0,  0, 25, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 26,  0,  0,  0, 52, 53, 53, 54,  0,  0,  0,  0, 52, 53},
    {53, 53, 54,  0,  0,  0,  0,  0, 52, 53, 53, 54,  0,  0,  0, 52, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 54,  0,  0,  0, 52, 53, 53, 54,  0,  0,  0,  0, 52, 53},
    {53, 53, 54,  0,  0,  0,  0,  0, 52, 53, 53, 54,  0,  0,  0, 52, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 54,  0,  0,  0, 52, 53, 53, 54,  0,  0,  0,  0, 52, 53},
    {53, 53, 54,  0,  0,  0,  0,  0, 52, 53, 53, 54,  0,  0,  0, 52, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 54,  0,  0,  0, 52, 53, 53, 54,  0,  0,  0,  0, 52, 53},
    {53, 53, 54,  0,  0,  0,  0,  0, 41, 48, 48, 55,  0,  0,  0, 52, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 54,  0,  0,  0, 41, 48, 48, 55,  0,  0,  0,  0, 52, 53},
    {53, 53, 54,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 41, 27, 48, 32, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 32, 48, 27, 55,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 52, 53},
    {53, 53, 54,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 52, 53},
    {53, 53, 54,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 52, 53},
    {53, 53, 54,  0,  0,  0,  0,  0, 49, 50, 50, 51,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 49, 50, 50, 51,  0,  0,  0,  0, 52, 53},
    {53, 53, 54,  0,  0,  0,  0,  0, 41, 53, 53, 54,  0,  0,  0, 49, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 51,  0,  0,  0, 52, 53, 53, 55,  0,  0,  0,  0, 52, 53},
    {53, 53, 54,  0,  0,  0,  0,  0,  0, 41, 53, 54,  0,  0,  0, 41, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 55,  0,  0,  0, 52, 53, 55,  0,  0,  0,  0,  0, 52, 53},
    {53, 53, 54,  0,  0,  0,  0,  0,  0,  0, 41, 55,  0,  0,  0,  0, 41, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 55,  0,  0,  0,  0, 41, 55,  0,  0,  0,  0,  0,  0, 52, 53},
    {53, 53, 54,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 52, 53},
    {53, 53, 54,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 52, 53},
    {53, 53, 54,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 52, 53},
    {53, 53, 53, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 51,  0,  0,  0,  0, 49, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 53, 53},
    };
    return obstacle_map;
}