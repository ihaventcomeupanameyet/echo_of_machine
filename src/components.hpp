#pragma once

#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include "common.hpp"
#include <vector>
#include <unordered_map>
#include "../ext/stb_image/stb_image.h"
#include "inventory.hpp"
#include "tileset.hpp"


#include "../ext/json.hpp"

using json = nlohmann::json;

enum class AnimationState {
	IDLE = 0,
	ATTACK,
	BLOCK,
	DEAD,
	PROJ,
	SECOND,
	WALK
};

enum class RobotState {
	WALK = 0,    
	IDLE = 1,
	DEAD = 2,
	HURT = 3,
	ATTACK = 4
};

enum class BossRobotState {
	ATTACK = 0,
	IDLE  = 1,
	WALK = 2
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
		case AnimationState::PROJ: return 6;
		case AnimationState::SECOND: return 7;
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

		if (current_state == AnimationState::WALK && (newState == AnimationState::ATTACK || newState == AnimationState::BLOCK || newState == AnimationState::SECOND || newState == AnimationState::PROJ)) {
			is_walking = true;
			can_attack = true;
		}

		if ((current_state == AnimationState::ATTACK || current_state == AnimationState::BLOCK || newState == AnimationState::SECOND || newState == AnimationState::PROJ) && can_attack) {
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

			if (current_state == AnimationState::ATTACK || current_state == AnimationState::BLOCK || current_state == AnimationState::SECOND || current_state == AnimationState::PROJ) {
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


enum class IceRobotState {
	WALK = 0,
	ATTACK = 1,
	IDLE = 2,
	DEAD = 3
};

class IceRobotAnimation : public BaseAnimation {

public:
	IceRobotAnimation(int sprite_size = 64, int s_width = 832, int s_height = 1024)
		: BaseAnimation(sprite_size, s_width, s_height) {}


	IceRobotState current_state = IceRobotState::IDLE;
	Direction current_dir = Direction::RIGHT;
	bool is_moving = false;

	int getMaxFrames() const override {
		switch (current_state) {
		case IceRobotState::WALK: return 9;
		case IceRobotState::IDLE: return 5;
		case IceRobotState::DEAD: return 5;
		case IceRobotState::ATTACK: return 13;

		default: return 0;
		}
	}

	bool loop() const override {
		return current_state == IceRobotState::WALK;
	}

	int getRow() const override {
		int state_off = static_cast<int>(current_state) * 4;
		int dir_off = static_cast<int>(current_dir);
		int row = state_off + dir_off;

		return row;
	}

	void setState(IceRobotState newState, Direction newDir) {
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

class BossRobotAnimation : public BaseAnimation {
public:
    BossRobotAnimation(int sprite_size = 128, int s_width = 1792, int s_height = 384)
        : BaseAnimation(sprite_size, s_width, s_height) {}

    BossRobotState current_state = BossRobotState::IDLE;
    Direction current_dir = Direction::RIGHT;

    int getMaxFrames() const override {
        switch (current_state) {
            case BossRobotState::ATTACK: return 12;
			case BossRobotState::WALK: return 14;
            case BossRobotState::IDLE: return 10;
            default: return 0;
        }
    }

    bool loop() const override {
        return current_state == BossRobotState::WALK || current_state == BossRobotState::IDLE || current_state == BossRobotState::ATTACK;
    }

    int getRow() const override {
        int state_off = static_cast<int>(current_state) * 4;
        int dir_off = static_cast<int>(current_dir);
        return state_off + dir_off;
    }

    void setState(BossRobotState newState, Direction newDir) {
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

            int max_frames = getMaxFrames();
            if (current_frame >= max_frames) {
                if (loop()) {
                    current_frame = 0;
                } else {
                    current_frame = max_frames - 1;
                }
            }
        }
    }
};


class DoorAnimation {
public:
	static constexpr float FRAME_TIME = 0.2f;
	int sprite_size;
	int s_width;
	int s_height;
	float current_frame_time = 0.f;
	int current_frame = 0;
	bool is_opening = false;

	DoorAnimation(int sprite_size = 128, int s_width = 768, int s_height = 128)
		: sprite_size(sprite_size), s_width(s_width), s_height(s_height) {
		current_frame = 0;
		current_frame_time = 0.f;
	}

	std::pair<vec2, vec2> getCurrentTexCoords() const {
		const float frame_width = 1.0f / 6.0f;  

		vec2 top_left = {
			frame_width * current_frame, 
			0.0f              
		};

		vec2 bottom_right = {
			frame_width * (current_frame + 1),  
			1.0f                       
		};

		return { top_left, bottom_right };
	}

	void update(float elapsed_ms) {
		if (!is_opening) return;

		current_frame_time += elapsed_ms / 1000.f;
		if (current_frame_time >= FRAME_TIME) {
			current_frame_time = 0;
			if (current_frame < 5) { 
				current_frame++;
			}
		}
	}
};

struct Door {
    bool is_right_door = true;
    bool is_locked = true;
    bool is_open = false;
	bool in_range = false;
};

struct Particle {
	float lifetime = 0.f;
	float max_lifetime = 3.0f;
	float opacity = 1.f;
	float size = 15.f;
};


// Player component
struct Player
{
	Inventory inventory;
	float speed = 150.f;
	float current_health = 100.f;  // Current health value
	float max_health = 100.f;      // Max health value
	float current_stamina = 100.f;
	float max_stamina = 100.f;
	float can_sprint = true;


	bool slow = false;
	float slow_count_down = 0.f;
	int armor_stat = 30; // armor
	int weapon_stat = 10; // damage done to enemies

	float dashSpeed = 200.f;
	vec2 dashTarget;
	vec2 dashStartPosition;
	bool dashTargetSet;
	float dashDuration = 0.5f; // Dash lasts 0.5 seconds by default
	vec2 lastDashDirection = vec2(0.f, 0.f);
	float dashTimer = 0.f;
	float dashCooldown = 0.f;
	bool isDashing = false;
};

// deault robot type
struct Robot
{
	// need to have health - after attacking - how much damage does it do.
	float current_health = 30;  // Current health value
	float max_health = 30;      // Max health value
	bool should_die = false;
	float death_cd;
	bool ice_proj = false;

	bool isCapturable = false; 
	bool showCaptureUI = false;
	float speed = 100.0f;
	float attack = 10.0f; 
	float max_attack = 20.0f;
	float max_speed = 150.0f;
	vec2 search_box;
	vec2 attack_box;
	vec2 panic_box;

	bool companion = false;
	std::vector<Item> disassembleItems;
};

struct BossRobot
{
	float current_health = 350; 
	float max_health = 350;
	bool should_die = false;
	float death_cd;

	vec2 search_box;
	vec2 attack_box;
	vec2 panic_box;
};

struct projectile {
	int dmg = 0;
	bool ice;
	bool friendly = false;
};

struct bossProjectile {
	int dmg = 0;
	float amplitude = 0.0f;
    float frequency = 1.0f;
    float time = 0.0f; 
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

	bool is_nighttime = false; 
	float nighttime_factor = 0.0f;
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


// Cutscene control 
struct Cutscene {
	bool is_active = false;                 // play cutscene
	float duration = 0.0f;                  // cutscene duration
	float current_time = 0.0f;              // timer��record current time
	std::vector<std::function<void(float)>> actions; // actions with time paramenter
	bool camera_control_enabled = true;     // camera control
	vec2 camera_target_position;            // real time update camera for target position
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
	BOSS_ROBOT,
	PLAYER_IDLE,
	PLAYER_FULLSHEET,
	CROCKBOT_FULLSHEET,
	BOSS_FULLSHEET,
	RIGHTDOORSHEET,
	BOTTOMDOORSHEET,
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
	CAPTURE_UI,
	C_BUTTON,
	C_BUTTON_HOVER,
	D_BUTTON,
	D_BUTTON_HOVER,
	PROJECTILE,
	ICE_PROJ,
	SMOKE,
	COMPANION_CROCKBOT,
	COMPANION_CROCKBOT_FULLSHEET,
	ROBOT_PART,
	ENERGY_CORE,
	TELEPORTER,
	START_SCREEN,
	ARMOR_ICON,
	WEAPON_ICON,
	ICE_ROBOT_FULLSHEET,
	ICE_ROBOT,
	COMPANION_ICE_ROBOT_FULLSHEET,
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

void from_json(const json& j, attackBox& box);
void to_json(json& j, const attackBox& box);
void from_json(const json& j, vec2& v);
void to_json(json& j, const vec2& v);
void to_json(json& j, const Motion& motion);
void from_json(const json& j, Motion& motion);
void to_json(json& j, const DeathTimer& timer);
void from_json(const json& j, DeathTimer& timer);
void to_json(json& j, const Collision& collision);
void from_json(const json& j, Collision& collision);
void to_json(json& j, const Player& player);
void from_json(const json& j, Player& player);
void to_json(json& j, const BaseAnimation& anim);
void from_json(const json& j, BaseAnimation& anim);
void to_json(json& j, const RobotAnimation& anim);
void from_json(const json& j, RobotAnimation& anim);
void to_json(json& j, const PlayerAnimation& anim);
void from_json(const json& j, PlayerAnimation& anim);
void to_json(json& j, const RenderRequest& request);
void from_json(const json& j, RenderRequest& request);

void to_json(json& j, const ScreenState& state);
void from_json(const json& j, ScreenState& state);

void to_json(json& j, const Robot& robot);
void from_json(const json& j, Robot& robot);
void to_json(json& j, const TileData& tileData);
void from_json(const json& j, TileData& tileData);
void to_json(json& j, const Tile& tile);
void from_json(const json& j, Tile& tile);
void to_json(json& j, const TileSet& tileset);
void from_json(const json& j, TileSet& tileset);
void to_json(json& j, const TileSetComponent& TileSetComponent);
void from_json(const json& j, TileSetComponent& TileSetComponent);
void to_json(json& j, const Key& key);
void from_json(const json& j, Key& key);
void to_json(json& j, const ArmorPlate& ap);
void from_json(const json& j, ArmorPlate& ap);
void to_json(json& j, const Potion& ap);
void from_json(const json& j, Potion& ap);
void to_json(json& j, const DebugComponent& ap);
void from_json(const json& j, DebugComponent& ap);
namespace glm {

	inline void to_json(nlohmann::json& j, const glm::vec3& v) {
		j = { v.x, v.y, v.z };
	}

	inline void from_json(const nlohmann::json& j, glm::vec3& v) {
		if (j.is_array() && j.size() == 3) {
			v.x = j[0].get<float>();
			v.y = j[1].get<float>();
			v.z = j[2].get<float>();
		}
		else {
			throw std::invalid_argument("JSON does not contain a valid glm::vec3 array");
		}
	}
}

void to_json(json& j, const T_map& t_map);
void from_json(const json& j, T_map& t_map);
void to_json(json& j, const Spaceship& ap);
void from_json(const json& j, Spaceship& ap);
void to_json(nlohmann::json& j, const projectile& p);
void from_json(const nlohmann::json& j, projectile& p);
void to_json(json& j, const Cutscene& cutscenes);
void from_json(const json& j, Cutscene& cutscenes);
#endif
