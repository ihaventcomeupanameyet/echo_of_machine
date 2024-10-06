#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "render_system.hpp"

// These are hardcoded to the dimensions of the entity texture
// BB = bounding box
const float ROBOT_BB_WIDTH   = 0.5f * 300.f;	// 1001
const float ROBOT_BB_HEIGHT  = 0.5f * 202.f;	// 870
const float PLAYER_BB_WIDTH = 0.4f * 300.f;	// 1001
const float PLAYER_BB_HEIGHT = 0.4f * 202.f;

// the player
Entity createPlayer(RenderSystem* renderer, vec2 pos);

// the enemy
Entity createRobot(RenderSystem* renderer, vec2 position);

// a red line for debugging purposes
Entity createLine(vec2 position, vec2 size);