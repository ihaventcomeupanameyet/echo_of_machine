#pragma once
#include "common.hpp"
#include <vector>
#include <unordered_map>
#include "../ext/stb_image/stb_image.h"
#include "inventory.hpp"
#include "tileset.hpp"

// Player component
struct Player
{
	Inventory inventory;
	float speed = 100.f;
	// need to add health
};

// anything that is deadly to the player
struct Robot
{
	// need to have health - after attacking - how much damage does it do.
};

struct Key
{

};

struct T_map {
	std::vector<std::vector<int>> tile_map;
	int tile_size = 0;
};



// All data relevant to the shape and motion of entities
struct Motion {
	vec2 position = { 0, 0 };
	vec2 position_s = { 0, 0 };
	vec2 position_e = { 0, 0 };
	bool should_move = false;
	float t_m = 0;
	float start_angle = 0;
	float angle = 0;
	float end_engle = 0;
	float t = 0;
	bool should_rotate = false;
	vec2 velocity = { 0, 0 };
	vec2 target_velocity = { 0, 0 };
	vec2 scale = { 10, 10 };
	bool is_stuck = false;
};

// Stucture to store collision information
struct Collision
{
	// Note, the first object is stored in the ECS container.entities
	Entity other; // the second object involved in the collision
	Collision(Entity& other) { this->other = other; };
};

// Data structure for toggling debug mode
struct Debug {
	bool in_debug_mode = 0;
	bool in_freeze_mode = 0;
};
extern Debug debugging;

// Sets the brightness of the screen
struct ScreenState
{
	float darken_screen_factor = -1;
	float fade_in_factor = 1.0f;   // Start fully dark (1 = black, 0 = fully transparent)
	bool fade_in_progress = true;
};

// A struct to refer to debugging graphics in the ECS
struct DebugComponent
{
	// Note, an empty struct has size 1
};
struct TileSetComponent {
	TileSet tileset;  // stores the actual TileSet
};

// A timer that will be associated to dying salmon
struct DeathTimer
{
	float counter_ms = 3000;
};

// TODO: review if we need this for our game
// Single Vertex Buffer element for non-textured meshes (coloured.vs.glsl & salmon.vs.glsl)
struct ColoredVertex
{
	vec3 position;
	vec3 color;
};

// Single Vertex Buffer element for textured sprites (textured.vs.glsl)
struct TexturedVertex
{
	vec3 position;
	vec2 texcoord;
};

// Mesh datastructure for storing vertex and index buffers
struct Mesh
{
	static bool loadFromOBJFile(std::string obj_path, std::vector<ColoredVertex>& out_vertices, std::vector<uint16_t>& out_vertex_indices, vec2& out_size);
	vec2 original_size = { 1,1 };
	std::vector<ColoredVertex> vertices;
	std::vector<uint16_t> vertex_indices;
};

/**
 * The following enumerators represent global identifiers refering to graphic
 * assets. For example TEXTURE_ASSET_ID are the identifiers of each texture
 * currently supported by the system.
 *
 * So, instead of referring to a game asset directly, the game logic just
 * uses these enumerators and the RenderRequest struct to inform the renderer
 * how to structure the next draw command.
 *
 * There are 2 reasons for this:
 *
 * First, game assets such as textures and meshes are large and should not be
 * copied around as this wastes memory and runtime. Thus separating the data
 * from its representation makes the system faster.
 *
 * Second, it is good practice to decouple the game logic from the render logic.
 * Imagine, for example, changing from OpenGL to Vulkan, if the game logic
 * depends on OpenGL semantics it will be much harder to do the switch than if
 * the renderer encapsulates all asset data and the game logic is agnostic to it.
 *
 * The final value in each enumeration is both a way to keep track of how many
 * enums there are, and as a default value to represent uninitialized fields.
 */


enum class TEXTURE_ASSET_ID {
	ROBOT = 0,
	PLAYER_IDLE,
	TILE_ATLAS,  // a single atlas for tiles
	TILE_ATLAS_LEVELS,
	KEY,
	TEXTURE_COUNT
};

const int texture_count = (int)TEXTURE_ASSET_ID::TEXTURE_COUNT;

enum class EFFECT_ASSET_ID {
	COLOURED = 0,
	TEXTURED = COLOURED + 1,
	SCREEN = TEXTURED + 1,
	EFFECT_COUNT = SCREEN + 1
};
const int effect_count = (int)EFFECT_ASSET_ID::EFFECT_COUNT;

enum class GEOMETRY_BUFFER_ID {
	SPRITE = 0,
	TILE = SPRITE + 1,
	DEBUG_LINE = TILE + 1,
	SCREEN_TRIANGLE = DEBUG_LINE + 1,
	GEOMETRY_COUNT = SCREEN_TRIANGLE + 1
};
const int geometry_count = (int)GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;

struct RenderRequest {
	TEXTURE_ASSET_ID used_texture = TEXTURE_ASSET_ID::TEXTURE_COUNT;
	EFFECT_ASSET_ID used_effect = EFFECT_ASSET_ID::EFFECT_COUNT;
	GEOMETRY_BUFFER_ID used_geometry = GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;
};

struct Tile
{
	int tileset_id;  // tileset it is linked to
	int tile_id;	//  ID for that tile in the tileset
	bool walkable;	// if the player can walk through the tile
	TileData tile_data; // coords
	TEXTURE_ASSET_ID atlas;
};