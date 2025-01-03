#pragma once
#pragma once
#include <unordered_map>
#include <glm/vec2.hpp>

//float map_width_px = 64 * map_width;
//float map_height_px = 64 * map_height;

struct TileData {
    glm::vec2 top_left;      // coordinates for the top-left corner of the tile in the atlas
    glm::vec2 bottom_right;  // coordinates for the bottom-right corner of the tile in the atlas
};

class TileSet {
public:
    // initialize the tile atlas
    void initializeTileTextureMap(int tiles_per_row, int tiles_per_col);

    // get texture coordinates for a given tile ID
    const TileData& getTileData(int tile_id) const;

    std::vector<std::vector<int>> initializeRemoteLocationMap();
    std::vector<std::vector<int>> initializeObstacleMap();

    int map_width = 50;  // Set your desired initial width
    int map_height = 30; // Set your desired initial height
    std::vector<std::vector<int>> TileSet::initializeTutorialLevelMap();
    std::vector<std::vector<int>> TileSet::initializeTutorialLevelObstacleMap();
    std::vector<std::vector<int>> TileSet::initializeFirstLevelMap();
    std::vector<std::vector<int>> TileSet::initializeFirstLevelObstacleMap();
    std::vector<std::vector<int>> TileSet::initializeSecondLevelMap();
    std::vector<std::vector<int>> TileSet::initializeSecondLevelObstacleMap();
    std::vector<std::vector<int>> TileSet::initializeThirdLevelMap();
    std::vector<std::vector<int>> TileSet::initializeThirdLevelObstacleMap();

    std::vector<std::vector<int>> TileSet::initializeFinalLevelMap();
    std::vector<std::vector<int>> TileSet::initializeFinalLevelObstacleMap();

    // map storing the texture coordinates
    std::unordered_map<int, TileData> tile_textures;

private:

};