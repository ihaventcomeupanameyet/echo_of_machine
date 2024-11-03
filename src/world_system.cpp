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
const size_t MAX_NUM_ROBOTS = 1; //15 originally
const size_t TOTAL_ROBOTS = 14;
const size_t ROBOT_SPAWN_DELAY_MS = 2000 * 3;
const size_t MAX_NUM_KEYS = 1;
const size_t KEY_SPAWN_DELAY = 8000;


// create the world
WorldSystem::WorldSystem()
	: points(0)
	, next_robot_spawn(0.f)
	, playerInventory(nullptr) {
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
	if (attack_sound != nullptr)
		Mix_FreeChunk(attack_sound);


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
	auto mouse_callback = [](GLFWwindow* window, int button, int action, int mods) {
		((WorldSystem*)glfwGetWindowUserPointer(window))->on_key(button, 0, action, mods);
		};
	glfwSetMouseButtonCallback(window, mouse_callback);
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
	attack_sound = Mix_LoadWAV(audio_path("attack_sound.wav").c_str());

	if (background_music == nullptr || player_dead_sound == nullptr || key_sound == nullptr || collision_sound == nullptr || attack_sound == nullptr) {
		fprintf(stderr, "Failed to load sounds\n %s\n %s\n %s\n make sure the data directory is present",
			audio_path("Galactic.wav").c_str(),
			audio_path("death_hq.wav").c_str(),
			audio_path("win.wav").c_str(),	
			audio_path("wall_contact.wav").c_str(),
			audio_path("attack_sound.wav").c_str());
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
	// TODO: move to init (?)
	if (registry.players.has(player)) {
		playerInventory = &registry.players.get(player).inventory;
	}
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
	// Update item dragging
	updateItemDragging();
	for (auto entity : registry.animations.entities) {
		auto& anim = registry.animations.get(entity);
		anim.update(elapsed_ms_since_last_update);
	}

	for (auto entity : registry.robotAnimations.entities) {
		auto& anim = registry.robotAnimations.get(entity);
		anim.update(elapsed_ms_since_last_update);
	}

	if (registry.players.has(player)) {
		Motion& player_motion = registry.motions.get(player);

		// Update camera to follow the player
		renderer->updateCameraPosition(player_motion.position);
	}

	float map_width_px = map_width * 64; // Assuming each tile is 64 pixels
	Motion& player_motion = registry.motions.get(player);
	if (player_motion.position.x >= map_width_px-100) {
		// Player has reached the right edge, trigger loading the new scene
		load_new_map();  // Call the new function to load the new tileset and map
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

	
	next_key_spawn -= elapsed_ms_since_last_update * current_speed;
	
	//registry.keys.components.size() <= MAX_NUM_KEYS &&

	if (!key_spawned &&  registry.robots.components.size() == 0 &&  total_robots_spawned == TOTAL_ROBOTS) {
		//&& next_key_spawn < 0.f 
		printf("Spawning key!\n");
		//next_key_spawn = (KEY_SPAWN_DELAY / 2) + uniform_dist(rng) * (KEY_SPAWN_DELAY / 2);

		//float spawn_area_width = window_width_px * 0.4f;
		//float spawn_area_height = window_height_px * 0.6f;
		//float spawn_area_left = (window_width_px - spawn_area_width) / 2;
		//float spawn_area_top = (window_height_px - spawn_area_height) / 2;

		//float spawn_x = spawn_area_left + uniform_dist(rng) * spawn_area_width;
		//float spawn_y = spawn_area_top + uniform_dist(rng) * spawn_area_height;

		createKey(renderer, {64.f * 46, 64.f * 3});
		key_spawned = true;
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

void WorldSystem::load_new_map() {
	// Clear all current entities and tiles
	for (auto entity : registry.motions.entities) {
		if (entity != player) {  // Skip removing the player entity
			registry.remove_all_components_of(entity);
		}
	}
	// Clear any previous tilesets
	registry.tilesets.clear();  // Clear the tilesets
	registry.tiles.clear();

	// Load a new tileset (for the new scene)
	auto new_tileset_entity = Entity();
	TileSetComponent& new_tileset_component = registry.tilesets.emplace(new_tileset_entity);
	new_tileset_component.tileset.initializeTileTextureMap(7, 15);  // Initialize with new tileset

	// Load the new grass and obstacle maps for the new scene
	std::vector<std::vector<int>> new_grass_map = new_tileset_component.tileset.initializeSecondLayerLandMap();
	std::vector<std::vector<int>> new_obstacle_map = new_tileset_component.tileset.initializeSecondLayerObstacleMap();

	// Set tile size (assumed to be 64)
	int tilesize = 64;

	// Render the new grass layer
	for (int y = 0; y < new_grass_map.size(); y++) {
		for (int x = 0; x < new_grass_map[y].size(); x++) {
			int tile_id = new_grass_map[y][x];
			vec2 position = { x * tilesize - (tilesize / 2) + tilesize, y * tilesize - (tilesize / 2) + tilesize };
			Entity tile_entity = createTileEntity(renderer, new_tileset_component.tileset, position, tilesize, tile_id);
			Tile& tile = registry.tiles.get(tile_entity);
			tile.walkable = true;  // Mark tiles as walkable
			tile.atlas = TEXTURE_ASSET_ID::TILE_ATLAS_LEVELS;  // Set new atlas for this tile
		}
	}

	// Render the new obstacle layer
	for (int y = 0; y < new_obstacle_map.size(); y++) {
		for (int x = 0; x < new_obstacle_map[y].size(); x++) {
			int tile_id = new_obstacle_map[y][x];
			if (tile_id != 0) {
				vec2 position = { x * tilesize - (tilesize / 2) + tilesize, y * tilesize - (tilesize / 2) + tilesize };
				Entity tile_entity = createTileEntity(renderer, new_tileset_component.tileset, position, tilesize, tile_id);
				Tile& tile = registry.tiles.get(tile_entity);
				tile.walkable = false;  // Mark as non-walkable
				tile.atlas = TEXTURE_ASSET_ID::TILE_ATLAS_LEVELS;  // Use the new tile atlas
			}
		}
	}

	// Respawn the player at the new starting position in the new scene
	float new_spawn_x = tilesize;  // Adjust the spawn position if necessary
	float new_spawn_y = tilesize * 2;
	Motion& player_motion = registry.motions.get(player);  // Get player's motion component
	player_motion.position = { new_spawn_x, new_spawn_y };


	// Update the camera to center on the player in the new map
	renderer->updateCameraPosition({ new_spawn_x, new_spawn_y });


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
	total_robots_spawned = 0;
	key_spawned = false;

	while (registry.motions.entities.size() > 0) {
		registry.remove_all_components_of(registry.motions.entities.back());
	}
	const std::vector<std::pair<float, float>> ROBOT_SPAWN_POSITIONS = {
	{64.f * 15, 64.f * 7},
	{64.f * 3, 64.f * 20},
	{64.f * 12, 64.f * 16},
	{64.f * 16, 64.f * 20},
	{64.f * 26, 64.f * 3},
	{64.f * 33, 64.f * 2},
	{64.f * 11, 64.f * 27},
	{64.f * 25, 64.f * 27},
	{64.f * 28, 64.f * 27},
	{64.f * 28, 64.f * 18},
	{64.f * 45, 64.f * 8},
	{64.f * 35, 64.f * 27},
	{64.f * 38, 64.f * 27},
	{64.f * 41, 64.f * 27}
	};
	//rintf("next_robot_spawn: %f\n", next_robot_spawn);

		//d::cout << "spawning robot!: " << registry.robots.components.size() << std::endl;
	if (registry.robots.components.size() <= MAX_NUM_ROBOTS && total_robots_spawned < TOTAL_ROBOTS) {
		// reset timer
		printf("Spawning robot!\n");
		next_robot_spawn = (ROBOT_SPAWN_DELAY_MS / 2) + uniform_dist(rng) * (ROBOT_SPAWN_DELAY_MS / 2);

		// create robots with random initial position

		//createRobot(renderer, vec2(window_width_px, 50.f + uniform_dist(rng) * (window_height_px - 100.f)));
		//total_robots_spawned++;

		const auto& pos = ROBOT_SPAWN_POSITIONS[total_robots_spawned];
		createRobot(renderer, vec2(pos.first, pos.second));
		total_robots_spawned++;
	}

	// initialize the grass tileset (base layer)
	auto grass_tileset_entity = Entity();
	TileSetComponent& grass_tileset_component = registry.tilesets.emplace(grass_tileset_entity);
	grass_tileset_component.tileset.initializeTileTextureMap(7, 15); // atlas size

	// initialize the obstacle tileset (second layer)
	auto obstacle_tileset_entity = Entity();
	TileSetComponent& obstacle_tileset_component = registry.tilesets.emplace(obstacle_tileset_entity);
	obstacle_tileset_component.tileset.initializeTileTextureMap(7, 15);

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
			tile.atlas = TEXTURE_ASSET_ID::TILE_ATLAS;
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
				tile.atlas = TEXTURE_ASSET_ID::TILE_ATLAS;
			}
		}
	}
	createTile_map(obstacle_map,tilesize);
	// Create the player entity
	float spawn_x = (map_width / 2) * tilesize;
	float spawn_y = (map_height / 2) * tilesize;


	/*player = createPlayer(renderer, { window_width_px / 2, window_height_px - 200 });*/

	// the orginal player position at level 1
	//player = createPlayer(renderer, { tilesize, tilesize * 8});

	// the player position at the remote location
	player = createPlayer(renderer, { tilesize * 8, tilesize * 15 });

	renderer->player = player;
	registry.colors.insert(player, { 1, 0.8f, 0.8f });

	spaceship = createSpaceship(renderer, { tilesize / 2, tilesize * 11 });

	// debug message
	std::cout << "spaceship created" << std::endl;

	registry.colors.insert(spaceship, {1, 0.8f, 0.8f});

	createPotion(renderer, { tilesize * 22, tilesize * 7 });
	createPotion(renderer, { tilesize * 18, tilesize * 27 });
	//createPotion(renderer, { tilesize * 39, tilesize * 11 });
	createArmorPlate(renderer, { tilesize * 39, tilesize * 11 });
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
				/*if (!registry.deathTimers.has(entity)) {
					// Scream, reset timer
					registry.deathTimers.emplace(entity);
					auto& animation = registry.animations.get(entity);
					animation.setState(AnimationState::DEAD, animation.current_dir);
					Mix_PlayChannel(-1, player_dead_sound, 0);

					/*registry.colors.get(entity) = glm::vec3(1.0f, 0.8f, 0.8f);*/
					/*Motion& motion = registry.motions.get(entity);
					motion.start_angle = 0.0f;
					motion.end_engle = -3.14 / 2;
					motion.should_rotate = true;
				}*/

			}

			if (registry.keys.has(entity_other)) {
				pickup_allowed = true;               // Allow pickup
				pickup_entity = entity_other;        // Set the entity to be picked up
				pickup_item_name = "Key";            // Set item name for inventory addition
			}

			// Check if the other entity is an armor plate
			if (registry.armorplates.has(entity_other)) {
				pickup_allowed = true;
				pickup_entity = entity_other;
				pickup_item_name = "ArmorPlate";
			}

			if (registry.potions.has(entity_other)) {
				pickup_allowed = true;               // Allow pickup
				pickup_entity = entity_other;        // Set the entity to be picked up
				pickup_item_name = "HealthPotion";            // Set item name for inventory addition
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

	static bool h_pressed = false;

	if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
		if (!h_pressed) {
			renderer->toggleHelp();
			game_paused = renderer->isHelpVisible();
			h_pressed = true;

			if (renderer->isHelpVisible()) {
				for (Entity entity : registry.motions.entities) {
					Motion& motion = registry.motions.get(entity);
					motion.target_velocity = { 0.0f, 0.0f };
					motion.velocity = { 0.0f, 0.0f };
				}
			}
		}
	}
	else {
		h_pressed = false;
		game_paused = false;
	}

	if (renderer->isHelpVisible()) {
		auto& animation = registry.animations.get(player);
		animation.is_walking = false;
		animation.setState(AnimationState::IDLE, animation.current_dir);

		return; 
	}

	auto& animation = registry.animations.get(player);
	Motion& motion = registry.motions.get(player);
	Inventory& inventory = registry.players.get(player).inventory;
	float playerSpeed = registry.players.get(player).speed;


	if (inventory.isOpen) {
		if (key == GLFW_MOUSE_BUTTON_LEFT) {

			onMouseClick(key, action, mod);  // Initiate dragging on left mouse click
		}
	}
	if (registry.deathTimers.has(player)) {
		// stop movement if player is dead
		motion.target_velocity = { 0.0f, 0.0f };
		return;
	}
	if (!inventory.isOpen) {
		if (key == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
			if (animation.current_state != AnimationState::ATTACK &&
				animation.current_state != AnimationState::BLOCK) {
				animation.setState(AnimationState::ATTACK, animation.current_dir);
				attackBox a;
				switch (animation.current_dir) {
				case Direction::DOWN:
					a = initAB(vec2(motion.position.x, motion.position.y + 48), vec2(64.f), 10, true);
					registry.attackbox.emplace_with_duplicates(player, a);
					Mix_PlayChannel(-1, attack_sound, 0);
				case Direction::UP:
					a = initAB(vec2(motion.position.x, motion.position.y - 48), vec2(64.f), 10, true);
					registry.attackbox.emplace_with_duplicates(player, a);
					Mix_PlayChannel(-1, attack_sound, 0);
				case Direction::LEFT:
					a = initAB(vec2(motion.position.x - 48, motion.position.y), vec2(64.f), 10, true);
					registry.attackbox.emplace_with_duplicates(player, a);
					Mix_PlayChannel(-1, attack_sound, 0);
				case Direction::RIGHT:
					a = initAB(vec2(motion.position.x + 48, motion.position.y), vec2(64.f), 10, true);
					registry.attackbox.emplace_with_duplicates(player, a);
					Mix_PlayChannel(-1, attack_sound, 0);
				}
			}
			return;
		}
	}
		// if changed to keyboard button (working while walking too)
		if (key == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
			if (animation.current_state != AnimationState::ATTACK &&
				animation.current_state != AnimationState::BLOCK) {
				animation.setState(AnimationState::BLOCK, animation.current_dir);
			}
			return;
		}
	

	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		switch (key) {
		case GLFW_KEY_F:
			// Toggle FPS counter display
			renderer->show_fps = !renderer->show_fps;
			printf("FPS counter %s\n", renderer->show_fps ? "enabled" : "disabled");
			break;
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
			if (pickup_allowed && pickup_entity != Entity{}) {
				// Add item to the player's inventory
			/*	if (registry.potions.has(pickup_entity)) {
					Player& player_data = registry.players.get(player);
					player_data.current_health += 30.f;
					if (player_data.current_health > 100.f) {
						player_data.current_health = 100.f;
					}
					printf("Player health: %f\n", player_data.current_health);
					registry.remove_all_components_of(pickup_entity);
					break;
				}*/
				Inventory& inventory = registry.players.get(player).inventory;
				inventory.addItem(pickup_item_name, 1);

				// Play the pickup sound
				Mix_PlayChannel(-1, key_sound, 0);


				// Remove the picked-up entity from the world
				registry.remove_all_components_of(pickup_entity);

				// Reset pickup flags
				pickup_allowed = false;
				pickup_entity = Entity{};
				pickup_item_name.clear();
			}
			break;
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(window, GL_TRUE);

			// Movement controls
		case GLFW_KEY_W:
			motion.target_velocity.y = -playerSpeed;
			animation.setState(AnimationState::WALK, Direction::UP);
			animation.is_walking = true;
			break;
		case GLFW_KEY_S:
			motion.target_velocity.y = playerSpeed;
			animation.setState(AnimationState::WALK, Direction::DOWN);
			animation.is_walking = true;
			break;
		case GLFW_KEY_A:
			motion.target_velocity.x = -playerSpeed;
			animation.setState(AnimationState::WALK, Direction::LEFT);
			animation.is_walking = true;
			break;
		case GLFW_KEY_D:
			motion.target_velocity.x = playerSpeed;
			animation.setState(AnimationState::WALK, Direction::RIGHT);
			animation.is_walking = true;
			break;

		case GLFW_KEY_MINUS:
		case GLFW_KEY_KP_SUBTRACT:
			if (registry.players.has(player)) {
				Player& player_data = registry.players.get(player);
				player_data.current_health -= 10.f;
				if (player_data.current_health < 0.f) {
					player_data.current_health = 0.f;
				}
				printf("Player health: %f\n", player_data.current_health);
			}
			break;
		}
	}
	else if (action == GLFW_RELEASE) {
		switch (key) {
		case GLFW_KEY_W:
		case GLFW_KEY_S:
			motion.target_velocity.y = 0.f;
			if (motion.target_velocity.x == 0.f) {
				animation.is_walking = false;
				animation.setState(AnimationState::IDLE, animation.current_dir);
			}
			break;
		case GLFW_KEY_A:
		case GLFW_KEY_D:
			motion.target_velocity.x = 0.f;
			if (motion.target_velocity.y == 0.f) {
				animation.is_walking = false;
				animation.setState(AnimationState::IDLE, animation.current_dir);
			}
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
	if (key == GLFW_KEY_TAB) {
		if (action == GLFW_RELEASE)
			debugging.in_debug_mode = false;
		else
			debugging.in_debug_mode = true;
	}
	// inventory slot selection
	if (key == GLFW_KEY_1) {
		inventory.setSelectedSlot(0);
	}
	else if (key == GLFW_KEY_2) {
		inventory.setSelectedSlot(1);
	}
	else if (key == GLFW_KEY_3) {
		inventory.setSelectedSlot(2);
	}
	
	// Use selected item in the active slot
	if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
		int selectedSlotIndex = inventory.getSelectedSlot(); // Get the index of the selected slot
		InventorySlot& selectedSlot = inventory.slots[selectedSlotIndex]; // Access the slot by index

		if (!selectedSlot.item.name.empty()) { // Check if an item is present in the slot
			inventory.useSelectedItem(); // Use the item in the selected slot
		}
		else {
			printf("No item in the selected slot.\n");
		}
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


void WorldSystem::on_mouse_move(glm::vec2 position) {
	renderer->mousePosition = position;
}
// world_system.cpp
void WorldSystem::onMouseClick(int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			renderer->mouseReleased = false;  // Reset on press

			for (int i = 0; i < playerInventory->getItems().size(); ++i) {
				vec2 slotPosition = renderer->getSlotPosition(i);
				vec2 slotSize = vec2(170.f, 100.f);

				if (renderer->mousePosition.x >= slotPosition.x && renderer->mousePosition.x <= slotPosition.x + slotSize.x &&
					renderer->mousePosition.y >= slotPosition.y && renderer->mousePosition.y <= slotPosition.y + slotSize.y) {
					renderer->isDragging = true;
					renderer->draggedSlot = i;
					renderer->dragOffset = renderer->mousePosition - slotPosition;
					break;
				}
			}
		}
		else if (action == GLFW_RELEASE && renderer->isDragging) {
			renderer->mouseReleased = true;  // Set on release

			for (int i = 0; i < playerInventory->getItems().size(); ++i) {
				vec2 targetSlotPosition = renderer->getSlotPosition(i);
				vec2 slotSize = vec2(170.f, 100.f);

				if (renderer->mousePosition.x >= targetSlotPosition.x && renderer->mousePosition.x <= targetSlotPosition.x + slotSize.x &&
					renderer->mousePosition.y >= targetSlotPosition.y && renderer->mousePosition.y <= targetSlotPosition.y + slotSize.y) {
					playerInventory->swapItems(renderer->draggedSlot, i);
					break;
				}
			}
			renderer->isDragging = false;
			renderer->draggedSlot = -1;
		}
	}
}

void WorldSystem::updateItemDragging() {
	if (!playerInventory || !playerInventory->isOpen) return;

	// If dragging, update the position
	if (renderer->isDragging && renderer->draggedSlot != -1) {
		// Calculate the current position for rendering the dragged item
		glm::vec2 draggedPosition = renderer->mousePosition - renderer->dragOffset;
		
	}
}