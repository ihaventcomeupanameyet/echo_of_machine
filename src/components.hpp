#pragma once
#include "common.hpp"
#include <vector>
#include <unordered_map>
#include "../ext/stb_image/stb_image.h"
#include "inventory.hpp"
#include "tileset.hpp"

enum class AnimationState {
	IDLE = 0,
	ATTACK,
	BLOCK,
	DEAD,
	WALK
};

enum class RobotState {
	WALK = 0,    
	IDLE = 1,
	DEAD = 2,
	HURT = 3,
	ATTACK = 4
};

enum class Direction {
	DOWN = 0,
	LEFT,
	RIGHT,
	UP
};

struct attackBox {
	vec2 position;
	vec2 bb;
	int dmg;
	bool friendly;
};

struct BaseAnimation {
	static constexpr float FRAME_TIME = 0.2f;

	int sprite_size;
	int s_width;
	int s_height;
	float current_frame_time = 0.f;
	int current_frame = 0;


public:
	BaseAnimation(int sprite_size = 64, int s_width = 448, int s_height = 1280) :
		sprite_size(sprite_size),
		s_width(s_width),
		s_height(s_height) {}

	Direction current_dir = Direction::RIGHT;
	virtual int getMaxFrames() const = 0;
	virtual bool loop() const = 0;
	virtual int getRow() const = 0;
	virtual void update(float elapsed_ms) = 0;

	std::pair<vec2, vec2> getCurrentTexCoords() const {
		int row = getRow();

		int frame_use = current_frame;

		float frame_width = static_cast<float>(sprite_size) / static_cast<float>(s_width);
		float frame_height = static_cast<float>(sprite_size) / static_cast<float>(s_height);

		vec2 top_left = {
			frame_use * frame_width,
			row * frame_height
		};

		vec2 bottom_right = {
			(frame_use + 1) * frame_width,
			(row + 1) * frame_height
		};

		return { top_left, bottom_right };
	}

	virtual ~BaseAnimation() {}
};

class PlayerAnimation : public BaseAnimation {


public:
	PlayerAnimation(int sprite_size = 64, int s_width = 448, int s_height = 1280)
		: BaseAnimation(sprite_size, s_width, s_height), can_attack(true) {}

	AnimationState current_state = AnimationState::IDLE;
	bool is_walking = false;
	bool can_attack;

	int getMaxFrames() const override {
		switch (current_state) {
		case AnimationState::IDLE: return 3;
		case AnimationState::ATTACK: return 7;
		case AnimationState::BLOCK: return 5;
		case AnimationState::DEAD: return 7;
		case AnimationState::WALK: return 5;
		default: return 0;
		}
	}

	bool loop() const override {
		return current_state == AnimationState::IDLE ||
			current_state == AnimationState::WALK;
	}

	int getRow() const override {
		int state_off = static_cast<int>(current_state) * 4;
		return state_off + static_cast<int>(current_dir);
	}

	void setState(AnimationState newState, Direction newDir) {

		if (current_state == AnimationState::WALK && (newState == AnimationState::ATTACK || newState == AnimationState::BLOCK)) {
			is_walking = true;
			can_attack = true;
		}

		if ((current_state == AnimationState::ATTACK || current_state == AnimationState::BLOCK) && can_attack) {
			if (newState == AnimationState::WALK) {
				current_dir = newDir;
				is_walking = true;
				return;
			}
		}

		if (newState != current_state || newDir != current_dir) {

			current_state = newState;
			current_dir = newDir;
			current_frame = 0;
			current_frame_time = 0;
		}
	}

	void update(float elapsed_ms) override{
		current_frame_time += elapsed_ms / 1000.f;

		if (current_frame_time >= FRAME_TIME) {
			current_frame_time = 0;
			/*current_frame++;*/

			if (!(current_state == AnimationState::DEAD && current_frame >= getMaxFrames() - 1)) {
				current_frame++;
			}

			int max_frames = getMaxFrames();

			if (current_state == AnimationState::ATTACK || current_state == AnimationState::BLOCK) {
				if (current_frame >= max_frames) {
					can_attack = false;
					setState(is_walking ? AnimationState::WALK : AnimationState::IDLE, current_dir);
				}
			}
			else if (current_frame >= max_frames) {
				if (loop()) {
					current_frame = 0;
				}
				else if (current_state != AnimationState::DEAD) {
					setState(AnimationState::IDLE, current_dir);
				}
				else {
					current_frame = max_frames - 1;
				}
			}
		}
	}
};

class RobotAnimation : public BaseAnimation {

public:
	RobotAnimation(int sprite_size = 64, int s_width = 640, int s_height = 1280)
		: BaseAnimation(sprite_size, s_width, s_height) {}


	RobotState current_state = RobotState::IDLE;
	Direction current_dir = Direction::RIGHT;
	bool is_moving = false;

	int getMaxFrames() const override {
		switch (current_state) {
		case RobotState::WALK: return 7;
		case RobotState::IDLE: return 4;
		case RobotState::DEAD: return 8;
		case RobotState::HURT: return 3;
		case RobotState::ATTACK: return 10;

		default: return 0;
		}
	}

	bool loop() const override {
		return current_state == RobotState::WALK;
	}

	int getRow() const override {
		int state_off = static_cast<int>(current_state) * 4;
		int dir_off = static_cast<int>(current_dir);
		int row = state_off + dir_off;

		return row;
	}

	void setState(RobotState newState, Direction newDir) {
		if (newState != current_state || newDir != current_dir) {
			current_state = newState;
			current_dir = newDir;
			current_frame = 0;
			current_frame_time = 0;
		}
	}

	void update(float elapsed_ms) override {
		current_frame_time += elapsed_ms / 1000.f;
		if (current_frame_time >= FRAME_TIME) {
			current_frame_time = 0;
			current_frame++;
			//std::cout << current_frame << std::endl;
			int max_frames = getMaxFrames();
			
			if (current_frame >= max_frames) {
				if (loop()) {
					current_frame = 0;
				}
				else {
					current_frame = max_frames - 1;
				}
			}
		}
	}
};


// Player component
struct Player
{
	Inventory inventory;
	float speed = 200.f;
	float current_health = 100.f;  // Current health value
	float max_health = 100.f;      // Max health value
	int armor_stat = 10; // damage done to enemies
	int weapon_stat = 10; // adds + health to player

};

// anything that is deadly to the player
struct Robot
{
	// need to have health - after attacking - how much damage does it do.
	float current_health = 30;  // Current health value
	float max_health = 30;      // Max health value
	bool should_die = false;
	float death_cd;
	bool isCapturable = false; 
	bool showCaptureUI = false;
	float speed = 100.0f;
	float attack = 10.0f; 
	vec2 search_box;
	vec2 attack_box;
	vec2 panic_box;
};

struct projectile {
	int dmg = 0;
};

struct Key
{

};

struct ArmorPlate
{

};

struct Potion {

};

struct T_map {
	std::vector<std::vector<int>> tile_map;
	int tile_size = 0;
};

struct Spaceship {

};

// font character structure
struct Character {
	unsigned int textureID;  // ID handle of the glyph texture
	glm::ivec2   size;       // Size of glyph
	glm::ivec2   bearing;    // Offset from baseline to left/top of glyph
	unsigned int advance;    // Offset to advance to next glyph
	char character;
};


// All data relevant to the shape and motion of entities
struct Motion {
	vec2 position = { 0, 0 };
	
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

	vec2 bb = vec2(0);
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
	PLAYER_FULLSHEET,
	CROCKBOT_FULLSHEET,
	HEALTHPOTION,
	TILE_ATLAS,  // a single atlas for tiles
	TILE_ATLAS_LEVELS,
	AVATAR,
	INVENTORY_SLOT,
	INVENTORY_SLOT_SELECTED,
	UI_SCREEN,
	PLAYER_UPGRADE_SLOT,
	KEY,
	ARMORPLATE,
	WEAPON_SLOT,
	ARMOR_SLOT,
	UPGRADE_BUTTON,
	UPGRADE_BUTTON_HOVER,
	PLAYER_AVATAR,
	INV_SLOT,
	// since there is no spaceship texture file in textures directory, so the game will be terminated when it cannot find the spaceship texture
	//SPACESHIP, 
	PROJECTILE,
	CAPTURE_UI,
	C_BUTTON,
	C_BUTTON_HOVER,
	D_BUTTON,
	D_BUTTON_HOVER,
	TEXTURE_COUNT
};

const int texture_count = (int)TEXTURE_ASSET_ID::TEXTURE_COUNT;

enum class EFFECT_ASSET_ID {
	COLOURED = 0,
	TEXTURED = COLOURED + 1,
	SCREEN = TEXTURED + 1,
	BOX = SCREEN + 1,
	FONT = BOX + 1,
	SPACESHIP = FONT + 1,
	EFFECT_COUNT = SPACESHIP + 1
};
const int effect_count = (int)EFFECT_ASSET_ID::EFFECT_COUNT;

enum class GEOMETRY_BUFFER_ID {
	SPRITE = 0,
	TILE = SPRITE + 1,
	DEBUG_LINE = TILE + 1,
	SCREEN_TRIANGLE = DEBUG_LINE + 1,
	SPACESHIP = SCREEN_TRIANGLE + 1,
	GEOMETRY_COUNT = SPACESHIP + 1
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