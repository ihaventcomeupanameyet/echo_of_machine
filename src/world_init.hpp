#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "render_system.hpp"
#include "tileset.hpp"
// These are hardcoded to the dimensions of the entity texture
// BB = bounding box
const float ROBOT_BB_WIDTH   = 0.5f * 300.f;	// 1001
const float ROBOT_BB_HEIGHT  = 0.5f * 202.f;	// 870
const float BOSS_ROBOT_BB_WIDTH   = 2.0f * 300.f;
const float BOSS_ROBOT_BB_HEIGHT  = 2.0f * 202.f;
const float PLAYER_BB_WIDTH = 0.4f * 300.f;	// 1001
const float PLAYER_BB_HEIGHT = 0.4f * 202.f;
const float KEY_BB_WIDTH = 0.2f * 200.f;
const float KEY_BB_HEIGHT = 0.2f * 200.f;
const float POTION_BB_WIDTH = 0.2f * 200.f;
const float POTION_BB_HEIGHT = 0.2f * 200.f;
const float ARMOR_BB_WIDTH = 0.2f * 200.f;
const float ARMOR_BB_HEIGHT = 0.2f * 200.f;

// the player
Entity createPlayer(RenderSystem* renderer, vec2 pos);

// the enemy
Entity createRobot(RenderSystem* renderer, vec2 position);
Entity createIceRobot(RenderSystem* renderer, vec2 position);
Entity createBossRobot(RenderSystem* renderer, vec2 position);

Entity createKey(RenderSystem* renderer, vec2 position);
Entity createCompanionRobot(RenderSystem* renderer, vec2 position, const Item& companionRobotItem);
Entity createPotion(RenderSystem* renderer, vec2 position);

Entity createArmorPlate(RenderSystem* renderer, vec2 position);

//Entity createBackgroundEntity(RenderSystem* renderer, vec2 position, vec2 scale);
Entity createTileEntity(RenderSystem* renderer, TileSet& tileset, vec2 position, float tile_size, int tile_id);

Entity createCompanionIceRobot(RenderSystem* renderer, vec2 position, const Item& companionRobotItem);

// a red line for debugging purposes
Entity createLine(vec2 position, vec2 size);

// For the DFS search
Entity createTile_map(std::vector<std::vector<int>> tile_map, int tile_size);

Entity createSpaceship(RenderSystem* renderer, vec2 pos);

attackBox initAB(vec2 pos, vec2 size, int dmg, bool friendly);

Entity createProjectile(vec2 position, vec2 speed, float angle,bool ice, bool player_projectile = false);

Entity createBossProjectile(vec2 position, vec2 speed, float angle, int dmg);

Entity createRightDoor(RenderSystem* renderer, vec2 position);

Entity createBottomDoor(RenderSystem* renderer, vec2 position);

Entity createSmokeParticle(RenderSystem* renderer, vec2 pos);