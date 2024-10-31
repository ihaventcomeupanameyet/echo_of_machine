#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "render_system.hpp"
#include "tileset.hpp"
// These are hardcoded to the dimensions of the entity texture
// BB = bounding box
const float ROBOT_BB_WIDTH   = 0.5f * 300.f;	// 1001
const float ROBOT_BB_HEIGHT  = 0.5f * 202.f;	// 870
const float PLAYER_BB_WIDTH = 0.4f * 300.f;	// 1001
const float PLAYER_BB_HEIGHT = 0.4f * 202.f;
const float KEY_BB_WIDTH = 1.0f * 10.f;
const float KEY_BB_HEIGHT = 1.0f * 28.f;


// the player
Entity createPlayer(RenderSystem* renderer, vec2 pos);

// the enemy
Entity createRobot(RenderSystem* renderer, vec2 position);

Entity createKey(RenderSystem* renderer, vec2 position);

//Entity createBackgroundEntity(RenderSystem* renderer, vec2 position, vec2 scale);
Entity createTileEntity(RenderSystem* renderer, TileSet& tileset, vec2 position, float tile_size, int tile_id);



// a red line for debugging purposes
Entity createLine(vec2 position, vec2 size);

// For the DFS search
Entity createTile_map(std::vector<std::vector<int>> tile_map, int tile_size);

attackBox initAB(vec2 pos, vec2 size, int dmg, bool friendly);
