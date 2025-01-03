#pragma once

// internal
#include "common.hpp"

// stlib
#include <vector>
#include <random>
#include <queue>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>

#include "render_system.hpp"
#include "ai_system.hpp"

#include "../ext/json.hpp"
using json = nlohmann::json;

// Container for all our entities and game logic. Individual rendering / update is
// deferred to the relative update() methods
class WorldSystem
{
public:

	bool introNotificationsAdded = false;
	bool armorPickedUp = false;
	bool potionPickedUp = false;
	bool movementHintShown = false;
	bool pickupHintShown = false;
	bool sprintHintShown = false;

	int robotPartsCount = 0;
	bool inventoryOpened = false;
	bool inventoryClosed = false;
	bool inventoryHintShown = false;
	bool attackNotificationsAdded = false;
	bool keyPickedUp = false;

	std::vector<std::vector<int>> obstacle_map;

	WorldSystem();

	// Creates a window
	GLFWwindow* create_window();

	// starts the game
	void init(RenderSystem* renderer);

	// Releases all associated resources
	~WorldSystem();

	// Steps the game ahead by ms milliseconds
	bool step(float elapsed_ms);

	// Check for collisions
	void handle_collisions();

	// Should the game be over ?
	bool is_over()const;

	Mix_Chunk* collision_sound;

	void play_collision_sound();

	void play_attack_sound();

	void play_ready_attack_sound();

	void play_death_sound();

	void play_awake_sound();

	void printInventory();
	bool  is_tile_walkable(vec2 position);

	// Method for updating item dragging
	void updateItemDragging();

	void updateParticles(float elapsed_ms);
	bool is_sprinting = false;
	float sprint_multiplyer = 2.f;

	// Mouse callbacks
	void onMouseClick(int button, int action, int mods);
	void on_mouse_move(vec2 pos);
	bool game_paused = false;
	bool game_over = false;
	bool uiScreenShown = false;
	Inventory* playerInventory; // Pointer to player's inventory for convenience
	bool isDragging = false;    // True if dragging an item
	int draggedSlot = -1;       // Index of the currently dragged slot
	glm::vec2 dragOffset;       // Offset for dragging to keep item centered
	glm::vec2 mousePosition;
	void WorldSystem::handleUpgradeButtonClick();
	void WorldSystem::handleCaptureButtonClick();
	void WorldSystem::handleDisassembleButtonClick();
	void WorldSystem::onMouseClickCaptureUI(int button, int action, int mods);
	void WorldSystem::useSelectedItem();
	vec2 WorldSystem::getPlayerPlacementPosition();
	bool WorldSystem::playerNearKey();
	void spawnBatSwarm(vec2 center, int count);
	// start screen
	bool show_start_screen = true;

	bool WorldSystem::hasPlayerMoved();

	bool WorldSystem::playerHasLeftStartingArea();
	bool WorldSystem::playerHasAttacked();

	std::queue<std::pair<std::string, float>> notificationQueue;

	TutorialState tutorial_state;
	bool player_control_enabled = true;


	int get_current_level() const { return current_level; }
	Entity get_player() const { return player; }
	Entity get_spaceship() const { return spaceship; }

	void set_current_level(int i) { current_level = i; }
	void set_player(Entity i) { player = i; }
	void set_spaceship(Entity i) { spaceship = i; }
	//bool get_key_handling() const { return key_handling; }
	//int get_current_level() const { return current_level; }

	void WorldSystem::triggerCutscene(const std::vector<TEXTURE_ASSET_ID>& images);
	void WorldSystem::end_game();

private:
	// Input callback functions
	void on_key(int key, int, int action, int mod);

	// restart level
	void restart_game();
	void WorldSystem::load_level(int level);

	bool WorldSystem::isKeyAllowed(int key)const;
	void load_second_level(int width, int height);
	void load_boss_level(int map_width, int map_height);
	void WorldSystem::load_third_level(int map_width, int map_height);
	void WorldSystem::load_tutorial_level(int map_width, int map_height);
	void load_remote_location(int width, int height);
	void load_first_level(int width, int height);
	void updateDoorAnimations(float elapsed_ms);
	bool hasNonCompanionRobots();
	void WorldSystem::restart_level();
	void WorldSystem::updateNotifications(float elapsed_ms);
	void WorldSystem::updateTutorialState();
	bool WorldSystem::playerNearArmor();
	bool WorldSystem::playerPickedUpArmor();
	bool WorldSystem::playerUsedArmor();
	bool WorldSystem::playerNearPotion();
	// OpenGL window handle
	GLFWwindow* window;
	int current_level = 1;
	const int MAX_LEVELS = 3;
	bool key_handling = false;
	bool armor_pickup_allowed = false;
	bool pickup_allowed = false;
	bool key_collected = false;
	Entity pickup_entity;

	std::string pickup_item_name;
	Entity armor_entity_to_pickup;
	bool pickupHintForE;
	bool hintForUsingItemsShown;
	// TODO M1: Consider removing as we do not have a point system in our game
	// Number of fish eaten by the salmon, displayed in the window title
	unsigned int points;

	// Game state
	RenderSystem* renderer;
	float current_speed;
	float next_robot_spawn;
	float next_boss_robot_spawn;
	float next_key_spawn;
	Entity player;
	size_t total_robots_spawned = 0;
	size_t total_boss_robots_spawned = 0;
	bool key_spawned = false;
	Entity spaceship;
	AISystem ai_system;


	// music references
	Mix_Music* background_music;
	Mix_Chunk* player_dead_sound;
	Mix_Chunk* key_sound;
	Mix_Chunk* attack_sound;
	Mix_Chunk* armor_break;
	Mix_Chunk* door_open;
	Mix_Chunk* robot_attack;
	Mix_Chunk* robot_ready_attack;
	Mix_Chunk* robot_death;
	Mix_Chunk* robot_awake;
	Mix_Chunk* Upgrade;
	Mix_Chunk* teleport_sound;
	Mix_Chunk* using_item;
	Mix_Chunk* insert_card;
	// C++ random number generator
	std::default_random_engine rng;
	std::uniform_real_distribution<float> uniform_dist; // number between 0..1
};

void to_json(json& j, const WorldSystem& ws);
void from_json(const json& j, WorldSystem& ws);


