// Header
#include "world_system.hpp"
#include "world_init.hpp"
#include "components.hpp"
#include "tileset.hpp"
#include "render_system.hpp"
#include "math_utils.hpp"

// stlib
#include <cassert>
#include <sstream>
#include <iostream>

#include "physics_system.hpp"

// Game configuration
const size_t MAX_NUM_ROBOTS = 15;
const size_t ROBOT_SPAWN_DELAY_MS = 2000 * 3;
const size_t MAX_NUM_KEYS = 5;
const size_t KEY_SPAWN_DELAY = 8000;


// create the world
WorldSystem::WorldSystem()
	: points(0)
	, next_robot_spawn(0.f) {
	// Seeding rng with random device
	rng = std::default_random_engine(std::random_device()());
}

WorldSystem::~WorldSystem() {

	// destroy music components
	if (background_music != nullptr)
		Mix_FreeMusic(background_music);
	if (player_dead_sound != nullptr)
		Mix_FreeChunk(player_dead_sound);
	if (key_sound != nullptr)
		Mix_FreeChunk(key_sound);
	if (collision_sound != nullptr)
		Mix_FreeChunk(collision_sound);


	Mix_CloseAudio();

	// Destroy all created components
	registry.clear_all_components();

	// Close the window
	glfwDestroyWindow(window);
}

// Debugging
namespace {
	void glfw_err_cb(int error, const char* desc) {
		fprintf(stderr, "%d: %s", error, desc);
	}
}

// World initialization
// Note, this has a lot of OpenGL specific things, could be moved to the renderer
GLFWwindow* WorldSystem::create_window() {
	///////////////////////////////////////
	// Initialize GLFW
	glfwSetErrorCallback(glfw_err_cb);
	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW");
		return nullptr;
	}

	//-------------------------------------------------------------------------
	// If you are on Linux or Windows, you can change these 2 numbers to 4 and 3 and
	// enable the glDebugMessageCallback to have OpenGL catch your mistakes for you.
	// GLFW / OGL Initialization
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#if __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	glfwWindowHint(GLFW_RESIZABLE, 0);

	// Create the main window (for rendering, keyboard, and mouse input)
	window = glfwCreateWindow(window_width_px, window_height_px, "Game Project", nullptr, nullptr);
	if (window == nullptr) {
		fprintf(stderr, "Failed to glfwCreateWindow");
		return nullptr;
	}

	// Setting callbacks to member functions (that's why the redirect is needed)
	// Input is handled using GLFW, for more info see
	// http://www.glfw.org/docs/latest/input_guide.html
	glfwSetWindowUserPointer(window, this);
	auto key_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2, int _3) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_key(_0, _1, _2, _3); };
	auto cursor_pos_redirect = [](GLFWwindow* wnd, double _0, double _1) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_mouse_move({ _0, _1 }); };
	glfwSetKeyCallback(window, key_redirect);
	glfwSetCursorPosCallback(window, cursor_pos_redirect);

	//////////////////////////////////////
	// Loading music and sounds with SDL
	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		fprintf(stderr, "Failed to initialize SDL Audio");
		return nullptr;
	}
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1) {
		fprintf(stderr, "Failed to open audio device");
		return nullptr;
	}

	background_music = Mix_LoadMUS(audio_path("Galactic.wav").c_str());
	player_dead_sound = Mix_LoadWAV(audio_path("death_hq.wav").c_str());
	key_sound = Mix_LoadWAV(audio_path("win.wav").c_str());
	collision_sound = Mix_LoadWAV(audio_path("wall_contact.wav").c_str());

	if (background_music == nullptr || player_dead_sound == nullptr || key_sound == nullptr || collision_sound == nullptr) {
		fprintf(stderr, "Failed to load sounds\n %s\n %s\n %s\n make sure the data directory is present",
			audio_path("Galactic.wav").c_str(),
			audio_path("death_hq.wav").c_str(),
			audio_path("win.wav").c_str(),	
			audio_path("wall_contact.wav").c_str());
		return nullptr;
	}

	return window;
}

void WorldSystem::init(RenderSystem* renderer_arg) {
	this->renderer = renderer_arg;
	// Playing background music indefinitely
	Mix_PlayMusic(background_music, -1);
	fprintf(stderr, "Loaded music\n");

	// Set all states to default
	restart_game();
}

void WorldSystem::play_collision_sound() {
	if (collision_sound) {
		Mix_PlayChannel(-1, collision_sound, 0);
	}
}

float lerp(float a, float b, float t) {
	return a + t * (b - a);
}

glm::vec3 lerp_color(glm::vec3 a, glm::vec3 b, float t) {
	return glm::vec3(lerp(a.x, b.x, t), lerp(a.y, b.y, t), lerp(a.z, b.z, t));
}



bool WorldSystem::step(float elapsed_ms_since_last_update) {
	// Updating window title with points
	std::stringstream title_ss;
	title_ss << "Points: " << points;
	glfwSetWindowTitle(window, title_ss.str().c_str());

	// Remove debug info from the last step
	while (registry.debugComponents.entities.size() > 0)
		registry.remove_all_components_of(registry.debugComponents.entities.back());

	   ScreenState& screen = registry.screenStates.components[0];

    if (screen.fade_in_progress) {
        // Reduce fade-in factor until it's fully transparent
        screen.fade_in_factor -= elapsed_ms_since_last_update / 3000.f;
        if (screen.fade_in_factor <= 0.f) {
            screen.fade_in_factor = 0.f;
            screen.fade_in_progress = false;
        }
    }

	// Removing out of screen entities
	auto& motions_registry = registry.motions;

	// Remove entities that leave the screen on the left side
	// Iterate backwards to be able to remove without unterfering with the next object to visit
	// (the containers exchange the last element with the current)
	for (int i = (int)motions_registry.components.size() - 1; i >= 0; --i) {
		Motion& motion = motions_registry.components[i];
		if (motion.position.x + abs(motion.scale.x) < 0.f) {
			if (!registry.players.has(motions_registry.entities[i])) // don't remove the player
				registry.remove_all_components_of(motions_registry.entities[i]);
		}
	}

	// spawn new robots
//rintf("elapsed_ms_since_last_update: %f, current_speed: %f\n", elapsed_ms_since_last_update, current_speed);

	next_robot_spawn -= elapsed_ms_since_last_update * current_speed;
	//rintf("next_robot_spawn: %f\n", next_robot_spawn);

		//d::cout << "spawning robot!: " << registry.robots.components.size() << std::endl;
	if (registry.robots.components.size() <= MAX_NUM_ROBOTS && next_robot_spawn < 0.f) {
		// reset timer
		printf("Spawning robot!\n");
		next_robot_spawn = (ROBOT_SPAWN_DELAY_MS / 2) + uniform_dist(rng) * (ROBOT_SPAWN_DELAY_MS / 2);

		// create robots with random initial position

		createRobot(renderer, vec2(window_width_px, 50.f + uniform_dist(rng) * (window_height_px - 100.f)));
	}

	next_key_spawn -= elapsed_ms_since_last_update * current_speed;

	if (registry.keys.components.size() <= MAX_NUM_KEYS && next_key_spawn < 0.f) {
		printf("Spawning key!\n");
		next_key_spawn = (KEY_SPAWN_DELAY / 2) + uniform_dist(rng) * (KEY_SPAWN_DELAY / 2);

		float spawn_area_width = window_width_px * 0.4f;
		float spawn_area_height = window_height_px * 0.6f;
		float spawn_area_left = (window_width_px - spawn_area_width) / 2;
		float spawn_area_top = (window_height_px - spawn_area_height) / 2;

		float spawn_x = spawn_area_left + uniform_dist(rng) * spawn_area_width;
		float spawn_y = spawn_area_top + uniform_dist(rng) * spawn_area_height;

		createKey(renderer, vec2(spawn_x, spawn_y));
	}


	// Processing the player state
	assert(registry.screenStates.components.size() <= 1);
//	ScreenState& screen = registry.screenStates.components[0];

	float min_counter_ms = 3000.f;
	for (Entity entity : registry.deathTimers.entities) {
		// progress timer
		DeathTimer& counter = registry.deathTimers.get(entity);
		counter.counter_ms -= elapsed_ms_since_last_update;
		if (counter.counter_ms < min_counter_ms) {
			min_counter_ms = counter.counter_ms;
		}

		float t = 1.0f - (counter.counter_ms / 3000.f);

		glm::vec3& color = registry.colors.get(entity);
		color = lerp_color(glm::vec3(1.0f, 0.8f, 0.8f), glm::vec3(1.0f, 0.0f, 0.0f), t);


		// restart the game once the death timer expired
		if (counter.counter_ms < 0) {
			registry.deathTimers.remove(entity);
			screen.darken_screen_factor = 0;
			restart_game();
			return true;
		}
	}
	// reduce window brightness if the player is dying
	screen.darken_screen_factor = 1 - min_counter_ms / 3000;

	return true;
}

// Reset the world state to its initial state
void WorldSystem::restart_game() {
	// reseting fade in
	ScreenState& screen = registry.screenStates.components[0];
	screen.fade_in_factor = 1.0f;  // Start fully black
	screen.fade_in_progress = true; // Start the fade-in process

	printf("Restarting\n");

	// Reset speed or any other game settings
	current_speed = 1.f;
	points = 0;

	while (registry.motions.entities.size() > 0) {
		registry.remove_all_components_of(registry.motions.entities.back());
	}

	// initialize the grass tileset (base layer)
	auto grass_tileset_entity = Entity();
	TileSetComponent& grass_tileset_component = registry.tilesets.emplace(grass_tileset_entity);
	grass_tileset_component.tileset.initializeTileTextureMap(7, 7); // atlas size

	// initialize the obstacle tileset (second layer)
	auto obstacle_tileset_entity = Entity();
	TileSetComponent& obstacle_tileset_component = registry.tilesets.emplace(obstacle_tileset_entity);
	obstacle_tileset_component.tileset.initializeTileTextureMap(7, 7);

	int tilesize = 64;

	std::vector<std::vector<int>> grass_map = grass_tileset_component.tileset.initializeGrassMap();
	std::vector<std::vector<int>> obstacle_map = obstacle_tileset_component.tileset.initializeObstacleMap();


	// render grass layer (base)
	for (int y = 0; y < map_height; y++) {
		for (int x = 0; x < map_width; x++) {
			int tile_id = grass_map[y][x];
			vec2 position = { x * tilesize - (tilesize / 2) + tilesize, y * tilesize - (tilesize / 2) + tilesize };
			Entity tile_entity = createTileEntity(renderer, grass_tileset_component.tileset, position, tilesize, tile_id);
			Tile& tile = registry.tiles.get(tile_entity);
			tile.walkable = true; // TODO: need to handle collision
		}
	}

	// render obstacle layer (second)
	for (int y = 0; y < obstacle_map.size(); y++) {
		for (int x = 0; x < obstacle_map[y].size(); x++) {
			int tile_id = obstacle_map[y][x];
			if (tile_id != 0) {
				vec2 position = { x * tilesize - (tilesize / 2) + tilesize, y * tilesize - (tilesize / 2) + tilesize };
				Entity tile_entity = createTileEntity(renderer, obstacle_tileset_component.tileset, position, tilesize, tile_id);

				Tile& tile = registry.tiles.get(tile_entity);
				tile.walkable = false; // TODO: need to handle collision
			}
		}
	}
	createTile_map(obstacle_map,tilesize);
	// Create the player entity
	player = createPlayer(renderer, { window_width_px / 2, window_height_px - 200 });
	renderer->player = player;
	registry.colors.insert(player, { 1, 0.8f, 0.8f });
}



// Compute collisions between entities
void WorldSystem::handle_collisions() {
	// Loop over all collisions detected by the physics system
	auto& collisionsRegistry = registry.collisions;

	for (uint i = 0; i < collisionsRegistry.components.size(); i++) {
		// The entity and its collider
		Entity entity = collisionsRegistry.entities[i];
		Entity entity_other = collisionsRegistry.components[i].other;


		// for now, we are only interested in collisions that involve the player
		if (registry.players.has(entity)) {
			//Player& player = registry.players.get(entity);

			Inventory& inventory = registry.players.get(entity).inventory;
			// Checking Player - Deadly collisions
			if (registry.robots.has(entity_other)) {
				// initiate death unless already dying
				if (!registry.deathTimers.has(entity)) {
					// Scream, reset timer
					registry.deathTimers.emplace(entity);
					Mix_PlayChannel(-1, player_dead_sound, 0);

					registry.colors.get(entity) = glm::vec3(1.0f, 0.8f, 0.8f);
					Motion motion = registry.motions.get(entity);
					motion.start_angle = 0.0f;
					motion.end_engle = 3.14 / 2;
					motion.should_rotate = true;
				}
			}

			else if (registry.keys.has(entity_other)) {
				if (key_handling) {
					if (!registry.deathTimers.has(entity)) {
						registry.remove_all_components_of(entity_other);
						inventory.addItem("Key", 1);
						Mix_PlayChannel(-1, key_sound, 0);
						++points;
						key_handling = false;
					}
				}
			}
		}
	}

	// Remove all collisions from this simulation step
	registry.collisions.clear();
}

// Should the game be over ?
bool WorldSystem::is_over() const {
	return bool(glfwWindowShouldClose(window));
}

// On key callback
void WorldSystem::on_key(int key, int, int action, int mod) {


	Motion& motion = registry.motions.get(player);
	Inventory& inventory = registry.players.get(player).inventory;
	float playerSpeed = registry.players.get(player).speed;
	if (registry.deathTimers.has(player)) {
		// stop movement if player is dead
		motion.target_velocity = { 0.0f, 0.0f };
		return;
	}

	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		switch (key) {
		case GLFW_KEY_I:
			// Toggle inventory open/close
			inventory.isOpen = !inventory.isOpen;
			if (inventory.isOpen) {
				inventory.display(); // display inventory contents in console
			}
			else {
				printf("Inventory closed.\n");
			}
			break;

		case GLFW_KEY_E:
			key_handling = true;
			break;
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(window, GL_TRUE);

			// Movement controls
		case GLFW_KEY_W:
			motion.target_velocity.y = -playerSpeed;
			break;
		case GLFW_KEY_S:
			motion.target_velocity.y = playerSpeed;
			break;
		case GLFW_KEY_A:
			motion.target_velocity.x = -playerSpeed;
			break;
		case GLFW_KEY_D:
			motion.target_velocity.x = playerSpeed;
			break;
		}
	}
	else if (action == GLFW_RELEASE) {
		switch (key) {
		case GLFW_KEY_W:
		case GLFW_KEY_S:
			motion.target_velocity.y = 0.f;
			break;
		case GLFW_KEY_A:
		case GLFW_KEY_D:
			motion.target_velocity.x = 0.f;
			break;
		case GLFW_KEY_E:
			key_handling = false;
			break;
		}
	}


	// Resetting game
	if (action == GLFW_RELEASE && key == GLFW_KEY_R) {
		int w, h;
		glfwGetWindowSize(window, &w, &h);

		restart_game();
	}

	// Debugging
	if (key == GLFW_KEY_D) {
		if (action == GLFW_RELEASE)
			debugging.in_debug_mode = false;
		else
			debugging.in_debug_mode = true;
	}

	// Control the current speed with `<` `>`
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) && key == GLFW_KEY_COMMA) {
		current_speed -= 0.1f;
		printf("Current speed = %f\n", current_speed);
	}
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) && key == GLFW_KEY_PERIOD) {
		current_speed += 0.1f;
		printf("Current speed = %f\n", current_speed);
	}
	current_speed = fmax(0.f, current_speed);
}


void WorldSystem::on_mouse_move(vec2 mouse_position) {

	(vec2)mouse_position; // dummy to avoid compiler warning
}
