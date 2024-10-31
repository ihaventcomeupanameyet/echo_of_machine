#pragma once

// internal
#include "common.hpp"

// stlib
#include <vector>
#include <random>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>

#include "render_system.hpp"

// Container for all our entities and game logic. Individual rendering / update is
// deferred to the relative update() methods
class WorldSystem
{
public:
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

	void printInventory();
	bool  is_tile_walkable(vec2 position);
	
	// Method for updating item dragging
	void updateItemDragging();

	// Mouse callbacks
	void onMouseClick(int button, int action, int mods);
	void on_mouse_move(vec2 pos);
	bool game_paused = false;

	Inventory* playerInventory; // Pointer to player's inventory for convenience
	bool isDragging = false;    // True if dragging an item
	int draggedSlot = -1;       // Index of the currently dragged slot
	glm::vec2 dragOffset;       // Offset for dragging to keep item centered
	glm::vec2 mousePosition;
private:
	// Input callback functions
	void on_key(int key, int, int action, int mod);
	
	// restart level
	void restart_game();

	void load_new_map();

	// OpenGL window handle
	GLFWwindow* window;

	bool key_handling = false;
	bool armor_pickup_allowed = false;
	bool pickup_allowed = false;
	Entity pickup_entity;
	std::string pickup_item_name;
	Entity armor_entity_to_pickup;
	// TODO M1: Consider removing as we do not have a point system in our game
	// Number of fish eaten by the salmon, displayed in the window title
	unsigned int points;

	// Game state
	RenderSystem* renderer;
	float current_speed;
	float next_robot_spawn;
	float next_key_spawn;
	Entity player;

	// music references
	Mix_Music* background_music;
	Mix_Chunk* player_dead_sound;
	Mix_Chunk* key_sound;

	// C++ random number generator
	std::default_random_engine rng;
	std::uniform_real_distribution<float> uniform_dist; // number between 0..1
};
