// Header
#include "world_system.hpp"
#include "world_init.hpp"
#include "components.hpp"
#include "tileset.hpp"
#include "render_system.hpp"
#include "math_utils.hpp"
#include "json.hpp"
// stlib
#include <cassert>
#include <sstream>
#include <iostream>

#include "physics_system.hpp"

// Game configuration
const size_t MAX_NUM_ROBOTS = 15; //15 originally
const size_t MAX_NUM_BOSS_ROBOTS = 1;
const size_t TOTAL_ROBOTS = 14;
const size_t TOTAL_BOSS_ROBOTS = 1;
const size_t ROBOT_SPAWN_DELAY_MS = 2000 * 3;
const size_t BOSS_ROBOT_SPAWN_DELAY_MS = 2000 * 3;
const size_t MAX_NUM_KEYS = 1;
const size_t KEY_SPAWN_DELAY = 8000;
constexpr float DOOR_INTERACTION_RANGE = 100.f;
const size_t MAX_PARTICLES = 20;


// create the world
WorldSystem::WorldSystem()
	: points(0)
	, next_robot_spawn(0.f)
	, next_boss_robot_spawn(0.f)
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
	if (armor_break != nullptr)
		Mix_FreeChunk(armor_break);

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
	armor_break = Mix_LoadWAV(audio_path("armor_break.wav").c_str());
	if (background_music == nullptr || player_dead_sound == nullptr || key_sound == nullptr || collision_sound == nullptr || attack_sound == nullptr) {
		fprintf(stderr, "Failed to load sounds\n %s\n %s\n %s\n make sure the data directory is present",
			audio_path("Galactic.wav").c_str(),
			audio_path("death_hq.wav").c_str(),
			audio_path("win.wav").c_str(),	
			audio_path("wall_contact.wav").c_str(),
			audio_path("attack_sound.wav").c_str(),
			audio_path("armor_break.wav").c_str());
		return nullptr;
	}

	return window;
}

void WorldSystem::init(RenderSystem* renderer_arg) {
	this->renderer = renderer_arg;
	this->renderer->show_start_screen = show_start_screen;
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

void WorldSystem::updateDoorAnimations(float elapsed_ms) {
	for (auto entity : registry.doors.entities) {
		Door& door = registry.doors.get(entity);
		DoorAnimation& animation = registry.doorAnimations.get(entity);

		if (animation.is_opening && !door.is_open) {
			animation.update(elapsed_ms);

			if (animation.current_frame == 5) { 
				door.is_open = true;
				animation.is_opening = false;
			}
		}
	}
}

void WorldSystem::updateParticles(float elapsed_ms) {
	if (registry.particles.entities.size() >= MAX_PARTICLES) {
		size_t to_remove = registry.particles.entities.size() - MAX_PARTICLES + 3;
		for (size_t i = 0; i < to_remove; i++) {
			if (!registry.particles.entities.empty()) {
				registry.remove_all_components_of(registry.particles.entities[0]);
			}
		}
	}

	for (unsigned int i = 0; i < registry.particles.entities.size(); i++) {
		Entity entity = registry.particles.entities[i];
		Motion& motion = registry.motions.get(entity);
		Particle& particle = registry.particles.get(entity);

		particle.lifetime += elapsed_ms / 1000.f;
		if (particle.lifetime >= particle.max_lifetime) {
			registry.remove_all_components_of(entity);
			continue;
		}

		float life_ratio = particle.lifetime / particle.max_lifetime;

		motion.velocity.y += (-2.0f - life_ratio * 2.0f) * (elapsed_ms / 1000.f);

		float wiggle = sin(particle.lifetime * 0.8f) * (10.f + life_ratio * 6.f);
		motion.velocity.x += wiggle * (elapsed_ms / 1000.f);

		motion.velocity *= 0.999f;

		motion.position += motion.velocity * (elapsed_ms / 1000.f);

		float size_increase = 2.0f;
		motion.scale = vec2(particle.size * (1.f + life_ratio * size_increase));

		float opacity_curve = 1.0f - (life_ratio * life_ratio * 0.8f);
		particle.opacity = std::max(0.0f, particle.opacity * opacity_curve);
	}
	static float spawn_timer = 0.f;
	spawn_timer += elapsed_ms;

	if (registry.spaceships.entities.size() > 0 && registry.particles.entities.size() < MAX_PARTICLES) {
		Entity spaceship_entity = registry.spaceships.entities[0];
		const Motion& spaceship_motion = registry.motions.get(spaceship_entity);

		if (spawn_timer >= 50.0f) {
			vec2 spawn_pos = spaceship_motion.position;
			spawn_pos.y += 60.f;
			spawn_pos.x -= 260.f;

			// clamped spawning
			size_t available_slots = MAX_PARTICLES - registry.particles.entities.size();
			size_t particles_to_spawn = std::min(size_t(3), available_slots);

			for (size_t i = 0; i < particles_to_spawn; i++) {
				vec2 offset = {
					static_cast<float>(rand() % 80 - 40),
					static_cast<float>(rand() % 20 - 10)
				};
				createSmokeParticle(renderer, spawn_pos + offset);
			}
			spawn_timer = 0.f;
		}
	}
}

bool WorldSystem::hasNonCompanionRobots() {
	for (auto entity : registry.robots.entities) {
		const Robot& robot = registry.robots.get(entity);
		if (!robot.companion) {
			return true;
		}
	}
	return false;
}

bool WorldSystem::step(float elapsed_ms_since_last_update) {
	// Updating window title with points
	/*std::stringstream title_ss;
	title_ss << "Points: " << points;
	glfwSetWindowTitle(window, title_ss.str().c_str());*/
	// TODO: move to init (?)
	if (registry.players.has(player)) {
		playerInventory = &registry.players.get(player).inventory;
	}
	uiScreenShown = false;

	for (auto entity : registry.robots.entities) {
		Robot& robot = registry.robots.get(entity);
		if (robot.showCaptureUI) {
			uiScreenShown = true;
			break;
		}
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
	Player& p = registry.players.get(player);
	if (is_sprinting) {
	
		if (p.current_stamina > 0.f) {
			float stamina_loss = 10.0f * elapsed_ms_since_last_update / 1000.f;
			p.current_stamina = std::max(0.f, p.current_stamina - stamina_loss);
		}
		// Stop sprinting if stamina runs out or `can_sprint` is false
    if (p.current_stamina <= 0.f || !p.can_sprint) {
        is_sprinting = false;

        // Reset motion speed to walking speed
        Motion& motion = registry.motions.get(player);
        float playerSpeed = p.speed;
        if (motion.target_velocity.x != 0.f) {
            motion.target_velocity.x = (motion.target_velocity.x > 0 ? 1.f : -1.f) * playerSpeed;
        }
        if (motion.target_velocity.y != 0.f) {
            motion.target_velocity.y = (motion.target_velocity.y > 0 ? 1.f : -1.f) * playerSpeed;
        }
	}
	} else {
		Player& p = registry.players.get(player);
		if (p.current_stamina < p.max_stamina) {
			float stamina_regen = 5.0f * elapsed_ms_since_last_update / 1000.f;
			p.current_stamina = std::min(p.max_stamina, p.current_stamina + stamina_regen);
		}
		if (p.current_stamina >= 0.25f * p.max_stamina) {
        p.can_sprint = true;
    }
	}
	if (screen.is_nighttime) {
			screen.nighttime_factor = 0.6f; 
	}
	else {
			screen.nighttime_factor = 0.0f; 
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

	for (auto entity : registry.iceRobotAnimations.entities) {
		auto& anim = registry.iceRobotAnimations.get(entity);
		anim.update(elapsed_ms_since_last_update);
	}

	for (auto entity : registry.bossRobotAnimations.entities) {
		auto& anim = registry.bossRobotAnimations.get(entity);
		anim.update(elapsed_ms_since_last_update);
	}

	if (registry.players.has(player)) {
		Motion& player_motion = registry.motions.get(player);

		// Update camera to follow the player
		renderer->updateCameraPosition(player_motion.position);
	}
	float map_height_px = map_height * 64;
	float map_width_px = map_width * 64;
	Motion& player_motion = registry.motions.get(player);


	for (Entity door_entity : registry.doors.entities) {
		Door& door = registry.doors.get(door_entity);
		Motion& door_motion = registry.motions.get(door_entity);

		float distance = glm::length(player_motion.position - door_motion.position);

		door.in_range = (distance < DOOR_INTERACTION_RANGE);
	}

	updateDoorAnimations(elapsed_ms_since_last_update);

	if ((current_level == 3 && player_motion.position.y >= map_height_px - 64) ||
		(current_level != 3 && player_motion.position.x >= map_width_px - 64)) {
		std::cout << "Current level: " << current_level << std::endl;

		// Check if the level requires a key to progress
		if ((current_level == 1) || current_level != 1) {
			current_level++;
			load_level(current_level);
			
			// Reset key_collected for the next level, if required
			key_collected = false;
		}
	}

	if (p.armor_stat == 0) {
		if (current_level == 2) {
			float health_loss = 2.0f * elapsed_ms_since_last_update / 6000.f;
			p.current_health = std::max(0.f, p.current_health - health_loss);
		} else if (current_level == 3) {
			float health_loss = 3.0f * elapsed_ms_since_last_update / 6000.f;
			p.current_health = std::max(0.f, p.current_health - health_loss);
		}
		else if (current_level == 4) {
			float health_loss = 4.0f * elapsed_ms_since_last_update / 6000.f;
			p.current_health = std::max(0.f, p.current_health - health_loss);
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

	
	next_key_spawn -= elapsed_ms_since_last_update * current_speed;
	
	if (!key_spawned && !hasNonCompanionRobots() && total_robots_spawned == TOTAL_ROBOTS) {
		//&& next_key_spawn < 0.f 
		printf("Spawning key!\n");

		if (current_level == 2) {

			createKey(renderer, { 64.f * 38, 64.f * 3 });
			key_spawned = true;
			renderer->key_spawned = true; //TODO 
		}

	

		if (current_level == 3) {
			createKey(renderer, { 64.f * 24, 64.f * 23.5 });
			key_spawned = true;
			renderer->key_spawned = true;
		}
	}

	// Processing the player state
	assert(registry.screenStates.components.size() <= 1);
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
		//	load_json(registry);
			//restart_game();
			Motion& player_motion = registry.motions.get(player);
			player_motion.velocity = vec2(0);
			player_motion.target_velocity = vec2(0);
			Player& player_data = registry.players.get(player);
			player_data.current_stamina = player_data.max_stamina;
			restart_game();
			return true;
		}
	}
	// reduce window brightness if the player is dying
	screen.darken_screen_factor = 1 - min_counter_ms / 3000;

	updateParticles(elapsed_ms_since_last_update);

	return true;
}

void WorldSystem::load_second_level(int map_width, int map_height) {
	// Clear all current entities and tiles
	
	for (auto entity : registry.motions.entities) {
		if (entity != player) {  // Skip removing the player entity
			registry.remove_all_components_of(entity);
		}
	}

	key_spawned = false;
	renderer->key_spawned = false;
	total_robots_spawned = 0;

	// Clear any previous tilesets
	registry.tilesets.clear();  // Clear the tilesets
	registry.tiles.clear();

	// Load a new tileset (for the new scene)
	auto new_tileset_entity = Entity();
	TileSetComponent& new_tileset_component = registry.tilesets.emplace(new_tileset_entity);
	new_tileset_component.tileset.initializeTileTextureMap(7, 43);  // Initialize with new tileset

	// Load the new grass and obstacle maps for the new scene
	std::vector<std::vector<int>> new_grass_map = new_tileset_component.tileset.initializeSecondLevelMap();
	std::vector<std::vector<int>> new_obstacle_map = new_tileset_component.tileset.initializeSecondLevelObstacleMap();


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

	createTile_map(new_obstacle_map, tilesize);

	float new_spawn_x = tilesize;  
	float new_spawn_y = tilesize * 2;
	Motion& player_motion = registry.motions.get(player); 
	player_motion.position = { new_spawn_x, new_spawn_y };

	createBottomDoor(renderer, { tilesize * 24, tilesize * 35});

	renderer->updateCameraPosition({ new_spawn_x, new_spawn_y });

	const std::vector<std::pair<float, float>> ROBOT_SPAWN_POSITIONS = {
	{64.f * 11, 64.f * 3},
	{64.f * 4, 64.f * 16},
	{64.f * 14, 64.f * 12},
	{64.f * 16, 64.f * 18},
	{64.f * 22, 64.f * 2},
	{64.f * 28, 64.f * 2},
	{64.f * 21, 64.f * 8},
	{64.f * 8, 64.f * 23},
	{64.f * 16, 64.f * 24},
	{64.f * 24, 64.f * 18},
	{64.f * 37, 64.f * 6},
	{64.f * 29, 64.f * 24},
	{64.f * 34, 64.f * 23},
	{64.f * 28, 64.f * 8}
	};

	for (size_t i = 0; i < ROBOT_SPAWN_POSITIONS.size(); i++) {
		if (registry.robots.components.size() >= MAX_NUM_ROBOTS) {
			break;
		}

	

		const auto& pos = ROBOT_SPAWN_POSITIONS[i];
		Entity new_robot = createIceRobot(renderer, vec2(pos.first, pos.second));
		Robot& robot = registry.robots.get(new_robot);

		std::uniform_int_distribution<int> attack_dist(7, robot.max_attack);
		std::uniform_int_distribution<int> speed_dist(90, robot.max_speed);

		robot.attack = attack_dist(rng);
		robot.speed = speed_dist(rng);

		if (i == 1 || i == 8) {
			robot.isCapturable = true;

			std::vector<Item> potential_items = Inventory::disassembleItems;
			std::shuffle(potential_items.begin(), potential_items.end(), rng);

			size_t added_items = 0;
			for (const Item& item : potential_items) {
				if (added_items >= 2) break;

				int min_drop = 1;
				int max_drop = 1;

				if (item.name == "Energy Core") {
					min_drop = 1; max_drop = 1;
				}
				else if (item.name == "Robot Parts") {
					min_drop = 1; max_drop = 3;
				}
				else if (item.name == "Teleporter") {
					min_drop = 1; max_drop = 3;
				}
				else if (item.name == "Armor Plate") {
					min_drop = 1; max_drop = 2;
				}

				int quantity = std::uniform_int_distribution<int>(min_drop, max_drop)(rng);

				if (quantity > 0) {
					robot.disassembleItems.emplace_back(item.name, quantity);
					++added_items;
				}
			}
		}
		total_robots_spawned++;
	}

	createPotion(renderer, { tilesize * 36, tilesize * 3 });
	createPotion(renderer, { tilesize * 24, tilesize * 8 });
	createArmorPlate(renderer, { tilesize * 20, tilesize * 18 });
}

void WorldSystem::load_boss_level(int map_width, int map_height) {
    // Clear all current entities and tiles
    for (auto entity : registry.motions.entities) {
        if (entity != player) {
            registry.remove_all_components_of(entity);
        }
    }

    // Clear any previous tilesets
    registry.tilesets.clear();  // Clear the tilesets
    registry.tiles.clear();

    // Load a new tileset (for the new scene)
    auto new_tileset_entity = Entity();
    TileSetComponent& new_tileset_component = registry.tilesets.emplace(new_tileset_entity);
    new_tileset_component.tileset.initializeTileTextureMap(7, 43);  // Initialize with new tileset

    // Load the new grass and obstacle maps for the new scene
    std::vector<std::vector<int>> new_grass_map = new_tileset_component.tileset.initializeFinalLevelMap();
    std::vector<std::vector<int>> new_obstacle_map = new_tileset_component.tileset.initializeFinalLevelObstacleMap();

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

    createTile_map(new_obstacle_map, tilesize);
    float new_spawn_x = tilesize * 43;  
    float new_spawn_y = tilesize * 2;
    Motion& player_motion = registry.motions.get(player); 
    player_motion.position = { new_spawn_x, new_spawn_y };

    // Update the camera to center on the player in the new map
    renderer->updateCameraPosition({ new_spawn_x, new_spawn_y });

    // Spawn the boss robot
    if (registry.bossRobots.components.size() < MAX_NUM_BOSS_ROBOTS) {
        printf("Spawning Boss Robot!\n");
        createBossRobot(renderer, { tilesize * 35, tilesize * 37 });
    } else {
        printf("Max number of boss robots already spawned.\n");
    }

	const std::vector<std::pair<float, float>> ROBOT_SPAWN_POSITIONS = {
	{64.f * 43, 64.f * 20},
	{64.f * 48, 64.f * 20},
	{64.f * 60, 64.f * 22},
	{64.f * 16, 64.f * 21},
	{64.f * 38, 64.f * 6},
	{64.f * 47, 64.f * 6}
	};

	for (size_t i = 0; i < ROBOT_SPAWN_POSITIONS.size(); i++) {
		if (registry.robots.components.size() >= MAX_NUM_ROBOTS) {
			break;
		}



		const auto& pos = ROBOT_SPAWN_POSITIONS[i];
		Entity new_robot = createRobot(renderer, vec2(pos.first, pos.second));
		Robot& robot = registry.robots.get(new_robot);

		std::uniform_int_distribution<int> attack_dist(7, robot.max_attack);
		std::uniform_int_distribution<int> speed_dist(90, robot.max_speed);

		robot.attack = attack_dist(rng);
		robot.speed = speed_dist(rng);
	}

	createPotion(renderer, { tilesize * 45, tilesize * 8 });
	createArmorPlate(renderer, { tilesize * 63, tilesize * 23 });
}

// Reset the world state to its initial state
void WorldSystem::restart_game() {
	// reseting fade in
	ScreenState& screen = registry.screenStates.components[0];
	screen.fade_in_factor = 1.0f;  // Start fully black
	screen.fade_in_progress = true; // Start the fade-in process

	printf("Restarting\n");
	renderer->show_capture_ui = false;
	uiScreenShown = false;
	// Reset speed or any other game settings
	current_speed = 1.f;
	points = 0;
	total_robots_spawned = 0;
	total_boss_robots_spawned = 0;
	key_spawned = false;
	renderer->game_paused = false;
	renderer->currentRobotEntity = Entity();
	game_paused = false;
	while (registry.motions.entities.size() > 0) {
		registry.remove_all_components_of(registry.motions.entities.back());
	}
	current_level = 1;
	load_level(current_level);
	
	
}

void WorldSystem::load_first_level(int map_width,int map_height) {

	/*createKey(renderer, { 64.f * 46, 64.f * 3 });
	key_spawned = true;
	renderer->key_spawned = true;*/
	printf("map_height: %d\n", map_height);
	printf("map_width: %d\n", map_width);

	const std::vector<std::pair<float, float>> ROBOT_SPAWN_POSITIONS = {
	{64.f * 15, 64.f * 7},
	{64.f * 4, 64.f * 17},
	{64.f * 13, 64.f * 13},
	{64.f * 16, 64.f * 17},
	{64.f * 19, 64.f * 2},
	{64.f * 25, 64.f * 2},
	{64.f * 11, 64.f * 23},
	{64.f * 23, 64.f * 19},
	{64.f * 24, 64.f * 23},
	{64.f * 28, 64.f * 16},
	{64.f * 37, 64.f * 8},
	{64.f * 31, 64.f * 23},
	{64.f * 34, 64.f * 23},
	{64.f * 37, 64.f * 23}
	};
	for (size_t i = total_robots_spawned; i < ROBOT_SPAWN_POSITIONS.size(); ++i) {
		if (registry.robots.components.size() >= MAX_NUM_ROBOTS) {
			break;
		}

		const auto& pos = ROBOT_SPAWN_POSITIONS[i];
		Entity new_robot = createRobot(renderer, vec2(pos.first, pos.second));
		Robot& robot = registry.robots.get(new_robot);

		std::uniform_int_distribution<int> attack_dist(7, robot.max_attack);
		std::uniform_int_distribution<int> speed_dist(90, robot.max_speed);

		robot.attack = attack_dist(rng);
		robot.speed = speed_dist(rng);

		if (i == 0 || i == 3 || i == 10) {
			robot.isCapturable = true;

			std::vector<Item> potential_items = Inventory::disassembleItems;
			std::shuffle(potential_items.begin(), potential_items.end(), rng);

			size_t added_items = 0;
			for (const Item& item : potential_items) {
				if (added_items >= 2) break;

				int min_drop = 1;
				int max_drop = 1;

				if (item.name == "Energy Core") {
					min_drop = 1; max_drop = 1;
				}
				else if (item.name == "Robot Parts") {
					min_drop = 1; max_drop = 3;
				}
				else if (item.name == "Teleporter") {
					min_drop = 1; max_drop = 3;
				}
				else if (item.name == "Armor Plate") {
					min_drop = 1; max_drop = 2;
				}

				int quantity = std::uniform_int_distribution<int>(min_drop, max_drop)(rng);

				if (quantity > 0) {
					robot.disassembleItems.emplace_back(item.name, quantity);
					++added_items;
				}
			}
		}

		total_robots_spawned++;
		if (total_robots_spawned >= TOTAL_ROBOTS) {
			break;
		}
	}


	// initialize the grass tileset (base layer)
	auto grass_tileset_entity = Entity();
	TileSetComponent& grass_tileset_component = registry.tilesets.emplace(grass_tileset_entity);
	grass_tileset_component.tileset.initializeTileTextureMap(7, 43); // atlas size

	int tilesize = 64;

	std::vector<std::vector<int>> grass_map = grass_tileset_component.tileset.initializeFirstLevelMap();
	std::vector<std::vector<int>> obstacle_map = grass_tileset_component.tileset.initializeFirstLevelObstacleMap();

	// render grass layer (base)
	printf("map_height: %d\n", grass_map.size());
	printf("map_width: %d\n", grass_map.size());

	for (int y = 0; y < grass_map.size(); y++) {
		for (int x = 0; x < grass_map[y].size(); x++) {
			int tile_id = grass_map[y][x];
			vec2 position = { x * tilesize - (tilesize / 2) + tilesize, y * tilesize - (tilesize / 2) + tilesize };
			Entity tile_entity = createTileEntity(renderer, grass_tileset_component.tileset, position, tilesize, tile_id);
			Tile& tile = registry.tiles.get(tile_entity);
			tile.walkable = true;
			tile.atlas = TEXTURE_ASSET_ID::TILE_ATLAS;
		}
	}

	// render obstacle layer (second)
	for (int y = 0; y < obstacle_map.size(); y++) {
		for (int x = 0; x < obstacle_map[y].size(); x++) {
			int tile_id = obstacle_map[y][x];
			if (tile_id != 0) {
				vec2 position = { x * tilesize - (tilesize / 2) + tilesize, y * tilesize - (tilesize / 2) + tilesize };
				Entity tile_entity = createTileEntity(renderer, grass_tileset_component.tileset, position, tilesize, tile_id);

				Tile& tile = registry.tiles.get(tile_entity);
				tile.walkable = false; 
				tile.atlas = TEXTURE_ASSET_ID::TILE_ATLAS;
			}
		}
	}
	createTile_map(obstacle_map, tilesize);
	// Create the player entity
	float spawn_x = (map_width / 2) * tilesize;
	float spawn_y = (map_height / 2) * tilesize;

	// Respawn the player at the new starting position in the new scene
	float new_spawn_x = tilesize;  // Adjust the spawn position if necessary
	float new_spawn_y = tilesize * 8;
	Motion& player_motion = registry.motions.get(player);  // Get player's motion component
	player_motion.position = { new_spawn_x, new_spawn_y };


	// Update the camera to center on the player in the new map
	renderer->updateCameraPosition({ new_spawn_x, new_spawn_y });

	createRightDoor(renderer, { tilesize * 49, tilesize * 3 });

	createPotion(renderer, { tilesize * 22, tilesize * 7 });
	createPotion(renderer, { tilesize * 18, tilesize * 23 });
	createArmorPlate(renderer, { tilesize * 31, tilesize * 11 });
}

void WorldSystem::load_remote_location(int map_width, int map_height) {
	auto spawn_tileset_entity = Entity();
	TileSetComponent& spawn_tileset_component = registry.tilesets.emplace(spawn_tileset_entity);
	spawn_tileset_component.tileset.initializeTileTextureMap(7, 43);

	int tilesize = 64;

	std::vector<std::vector<int>> grass_map = spawn_tileset_component.tileset.initializeRemoteLocationMap();
	std::vector<std::vector<int>> obstacle_map = spawn_tileset_component.tileset.initializeObstacleMap();


	// render grass layer (base)
	for (int y = 0; y < map_height; y++) {
		for (int x = 0; x < map_width; x++) {
			int tile_id = grass_map[y][x];
			vec2 position = { x * tilesize - (tilesize / 2) + tilesize, y * tilesize - (tilesize / 2) + tilesize };
			Entity tile_entity = createTileEntity(renderer, spawn_tileset_component.tileset, position, tilesize, tile_id);
			Tile& tile = registry.tiles.get(tile_entity);
			tile.walkable = true;
			tile.atlas = TEXTURE_ASSET_ID::TILE_ATLAS;
		}
	}

	// render obstacle layer (second)
	for (int y = 0; y < obstacle_map.size(); y++) {
		for (int x = 0; x < obstacle_map[y].size(); x++) {
			int tile_id = obstacle_map[y][x];
			if (tile_id != 0) {
				vec2 position = { x * tilesize - (tilesize / 2) + tilesize, y * tilesize - (tilesize / 2) + tilesize };
				Entity tile_entity = createTileEntity(renderer, spawn_tileset_component.tileset, position, tilesize, tile_id);

				Tile& tile = registry.tiles.get(tile_entity);
				tile.walkable = false;
				tile.atlas = TEXTURE_ASSET_ID::TILE_ATLAS;
			}
		}
	}
	createTile_map(obstacle_map, tilesize);
	// Create the player entity
	float spawn_x = (map_width / 2) * tilesize;
	float spawn_y = (map_height / 2) * tilesize;

	// the orginal player position at level 1
	player = createPlayer(renderer, { tilesize * 13, tilesize * 10});
	// the player position at the remote location
	//player = createPlayer(renderer, { tilesize * 15, tilesize * 15 });
	//createKey(renderer, { tilesize * 15, tilesize * 10 });
	//createKey(renderer, { tilesize * 7, tilesize * 10 });
	registry.colors.insert(player, glm::vec3(1.f, 1.f, 1.f));
	spaceship = createSpaceship(renderer, { tilesize * 7, tilesize * 10 });
	registry.colors.insert(spaceship, { 0.761f, 0.537f, 0.118f });
	//createPotion(renderer, { tilesize * 7, tilesize * 10 });
	//createPotion(renderer, { tilesize * 7, tilesize * 10 });
	//createArmorPlate(renderer, { tilesize * 7, tilesize * 10 });
	//createKey(renderer, { tilesize * 7, tilesize * 10 });
	renderer->player = player;

}
// Compute collisions between entities
void WorldSystem::handle_collisions() {
	// Loop over all collisions detected by the physics system
	pickup_allowed = false;
	pickup_entity = Entity{};
	pickup_item_name.clear();
	auto& collisionsRegistry = registry.collisions;

	for (uint i = 0; i < collisionsRegistry.components.size(); i++) {
		// The entity and its collider
		Entity entity = collisionsRegistry.entities[i];
		Entity entity_other = collisionsRegistry.components[i].other;


		if (registry.projectile.has(entity)) {
			projectile pj = registry.projectile.get(entity);

			if (pj.friendly) {
				if (registry.robots.has(entity_other)) {
					Robot& robot = registry.robots.get(entity_other);
					if (robot.isCapturable && robot.showCaptureUI) {
						return;
					}
					if (!robot.companion) {
						robot.current_health -= pj.dmg;
						//printf("Robot hit! Health remaining: %.2f\n", robot.current_health);

						registry.remove_all_components_of(entity);
					}
				
				}
				else if (registry.bossRobots.has(entity_other)) {
					BossRobot& boss = registry.bossRobots.get(entity_other);
					boss.current_health -= pj.dmg;
					registry.remove_all_components_of(entity);
					}
			}
			if (!pj.friendly && registry.robots.has(entity_other)) {
				Robot& robot = registry.robots.get(entity_other);
				if (robot.isCapturable && robot.showCaptureUI) {
					return;
				} else if (robot.companion) {
					robot.current_health -= pj.dmg;
					registry.remove_all_components_of(entity);
				}
			}
		}
		else if (registry.projectile.has(entity_other)) {
			projectile pj = registry.projectile.get(entity_other);

			if (pj.friendly) {
				if (registry.robots.has(entity)) {
					Robot& robot = registry.robots.get(entity);
					if (robot.isCapturable && robot.showCaptureUI) {
						return;
					}
					if (!robot.companion) {
						robot.current_health -= pj.dmg;
						//printf("Robot hit! Health remaining: %.2f\n", robot.current_health);

						registry.remove_all_components_of(entity_other);
					}
				}
				else if (registry.bossRobots.has(entity)) {
					BossRobot& boss = registry.bossRobots.get(entity);
					boss.current_health -= pj.dmg;
					registry.remove_all_components_of(entity_other);
				}
			}

			if (!pj.friendly && registry.robots.has(entity)) {
				Robot& robot = registry.robots.get(entity);
				if (robot.isCapturable && robot.showCaptureUI) {
					return;
				}else if (robot.companion) {
					robot.current_health -= pj.dmg;
					registry.remove_all_components_of(entity_other);
				}
			}
		}

		// Check if the boss projectile hits the player
		if (registry.bossProjectile.has(entity)) {
			bossProjectile& pj = registry.bossProjectile.get(entity);
			if (registry.players.has(entity_other)) {
				Player& player = registry.players.get(entity_other);
				player.current_health -= pj.dmg; // Apply damage to the player
				std::cout << "Player hit by boss projectile! Health remaining: " << player.current_health << std::endl;
				registry.remove_all_components_of(entity); // Remove the boss projectile
			}
		}

		// Check if a player projectile hits the boss robot
		if (registry.projectile.has(entity)) {
			projectile& pj = registry.projectile.get(entity);
			if (registry.bossRobots.has(entity_other)) {
				BossRobot& bossRobot = registry.bossRobots.get(entity_other);
				if (!pj.friendly) { // Ensure the projectile is not friendly
					bossRobot.current_health -= pj.dmg; // Apply damage to the boss robot
					if (bossRobot.current_health <= 0) {
						// Handle boss robot death
						registry.remove_all_components_of(entity_other); // Remove the boss robot entity
					}
					registry.remove_all_components_of(entity); // Remove the player projectile
				}
			}
		}

		// for now, we are only interested in collisions that involve the player
		if (registry.players.has(entity)) {
			//Player& player = registry.players.get(entity);

			Inventory& inventory = registry.players.get(entity).inventory;
			// Checking Player - Deadly collisions
			if (registry.robots.has(entity_other)) {
				Robot& robot = registry.robots.get(entity_other);
				if (robot.companion) {
					pickup_allowed = true;
					pickup_entity = entity_other;
					if (registry.iceRobotAnimations.has(entity_other)) {
						pickup_item_name = "IceRobot";
					} else pickup_item_name = "CompanionRobot";
				}

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

			if (registry.doors.has(entity_other)) {
				Door& door = registry.doors.get(entity_other);
				door.in_range = true;  // Player is in range of door


			}

			if (registry.projectile.has(entity_other)) {
				projectile& pj = registry.projectile.get(entity_other);

				if (registry.players.has(entity) && !pj.friendly) {
					Player& p = registry.players.get(entity);
					PlayerAnimation& pa = registry.animations.get(entity);
					if (pa.current_state != AnimationState::BLOCK) {
						if (p.current_health > 0) {
							if (p.armor_stat > 0) {
								float remaining_damage = pj.dmg - p.armor_stat;
								p.armor_stat -= pj.dmg;
								if (p.armor_stat <= 0) {
									p.armor_stat = 0;
									Mix_PlayChannel(-1, armor_break, 0);
								}
								if (remaining_damage > 0) {
									p.current_health = std::max(0.f, p.current_health - remaining_damage);
								}
							}
							else {
								p.current_health = std::max(0.f, p.current_health - pj.dmg);
							}
						}
						if (p.current_health <= 0) {
							if (!registry.deathTimers.has(entity)) {
								registry.deathTimers.emplace(entity);
								pa.setState(AnimationState::DEAD, pa.current_dir);
								Mix_PlayChannel(-1, player_dead_sound, 0);
							}
						}
					} 
					if (pj.ice) {
						p.slow_count_down = 1000.f;
						p.slow = true;
					}
					if (pa.current_state != AnimationState::BLOCK) {
						registry.remove_all_components_of(entity_other);
					}
					else {
						Motion& m = registry.motions.get(entity_other);
						m.target_velocity = -m.target_velocity;
						m.velocity = -m.velocity;
						m.angle += 3.14;
						pj.friendly = true;
					}
					//registry.remove_all_components_of(entity_other);
				}
			}

			if (registry.bossProjectile.has(entity_other)) {
				bossProjectile& pj = registry.bossProjectile.get(entity_other);

				if (registry.players.has(entity)) {
					Player& p = registry.players.get(entity);
					PlayerAnimation& pa = registry.animations.get(entity);
					if (pa.current_state != AnimationState::BLOCK) {
						if (p.current_health > 0) {
							if (p.armor_stat > 0) {
								float remaining_damage = pj.dmg - p.armor_stat;
								p.armor_stat -= pj.dmg;
								if (p.armor_stat <= 0) {
									p.armor_stat = 0;
									Mix_PlayChannel(-1, armor_break, 0);
								}
								if (remaining_damage > 0) {
									p.current_health = std::max(0.f, p.current_health - remaining_damage);
								}
							}
							else {
								p.current_health = std::max(0.f, p.current_health - pj.dmg);
							}
						}
						if (p.current_health <= 0) {
							if (!registry.deathTimers.has(entity)) {
								registry.deathTimers.emplace(entity);
								pa.setState(AnimationState::DEAD, pa.current_dir);
								Mix_PlayChannel(-1, player_dead_sound, 0);
							}
						}
					} 
					if (pa.current_state != AnimationState::BLOCK) {
						registry.remove_all_components_of(entity_other);
					}
					//registry.remove_all_components_of(entity_other);
			
					//registry.remove_all_components_of(entity_other);
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

	// start screen
	if (show_start_screen && key == GLFW_KEY_G && action == GLFW_PRESS) {
		show_start_screen = false;
		renderer->show_start_screen = false;
		return;
	}

	static bool h_pressed = false;
	if (renderer->game_paused && key == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		switch (renderer->hovered_menu_index) {
		case 0: // Resume
			printf("Resume selected.\n");
			game_paused = false;
			renderer->game_paused = false;
			break;
		case 1: // Help
			printf("Help selected.\n");
			game_paused = false;
			renderer->game_paused = false;
			renderer->toggleHelp();
			break;
		case 2: // Save and Quit
			printf("Save and Quit selected.\n");
			glfwSetWindowShouldClose(window, GL_TRUE);
			break;
		case 3: // Restart Game
			printf("Restart Game selected.\n");
			int w, h;
			glfwGetWindowSize(window, &w, &h);
			restart_game();
			break;
		default:
			printf("No valid menu item selected.\n");
			break;
		}
		return;
	}
	

	if (key == GLFW_KEY_LEFT_SHIFT) {
		// only spring if player.current_stamina > 0
		if (action == GLFW_PRESS) {
			Player& p = registry.players.get(player);

			if (p.can_sprint && p.current_stamina > 0.f) {
            is_sprinting = true;

            Motion& motion = registry.motions.get(player);
            float playerSpeed = p.speed * sprint_multiplyer;
            if (motion.target_velocity.x != 0.f) {
                motion.target_velocity.x = (motion.target_velocity.x > 0 ? 1.f : -1.f) * playerSpeed;
            }
            if (motion.target_velocity.y != 0.f) {
                motion.target_velocity.y = (motion.target_velocity.y > 0 ? 1.f : -1.f) * playerSpeed;
            }
        }
	}
		else if (action == GLFW_RELEASE) {
			is_sprinting = false;

			Motion& motion = registry.motions.get(player);
			float playerSpeed = registry.players.get(player).speed;
			if (motion.target_velocity.x != 0.f) {
				motion.target_velocity.x = (motion.target_velocity.x > 0 ? 1.f : -1.f) * playerSpeed;
			}
			if (motion.target_velocity.y != 0.f) {
				motion.target_velocity.y = (motion.target_velocity.y > 0 ? 1.f : -1.f) * playerSpeed;
			}
		}
	}

	/*if (key == GLFW_KEY_L) {
		load_json(registry);
		Motion& player_motion = registry.motions.get(player);
		player_motion.velocity = vec2(0);
		player_motion.target_velocity = vec2(0);
	}*/

	if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
		if (!h_pressed) {
			renderer->toggleHelp();
			//game_paused = renderer->isHelpVisible();
			h_pressed = true;
		}
	}
	else {
		h_pressed = false;
	//	game_paused = false;
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
	float playerSpeed = registry.players.get(player).speed * (is_sprinting ? sprint_multiplyer : 1.f);
	Player& player_data = registry.players.get(player);

	if (inventory.isOpen) {
		if (key == GLFW_MOUSE_BUTTON_LEFT) {

			onMouseClick(key, action, mod);  // Initiate dragging on left mouse click
		}
	}


	if (renderer->show_capture_ui){
		if (key == GLFW_MOUSE_BUTTON_LEFT) {
		//	game_paused = renderer->show_capture_ui;
			onMouseClickCaptureUI(key, action, mod);
		}
		if (registry.players.has(player)) {
			Motion& player_motion = registry.motions.get(player);
			player_motion.velocity = vec2(0.f, 0.f);
			player_motion.target_velocity = vec2(0.f, 0.f);
		}
	
			auto& animation = registry.animations.get(player);
			animation.is_walking = false;
			animation.setState(AnimationState::IDLE, animation.current_dir);

			return;
		
	}
	


	if (registry.deathTimers.has(player)) {
		// stop movement if player is dead
		motion.target_velocity = { 0.0f, 0.0f };
		return;
	}
	if (!inventory.isOpen && !renderer->show_capture_ui) {
		if (key == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
			if (animation.current_state != AnimationState::ATTACK &&
				animation.current_state != AnimationState::BLOCK) {
				Player& player_data = registry.players.get(player);
				animation.setState(AnimationState::ATTACK, animation.current_dir);
				attackBox a;
				switch (animation.current_dir) {
				case Direction::DOWN:
					a = initAB(vec2(motion.position.x, motion.position.y + 48), vec2(64.f), player_data.weapon_stat, true);
					registry.attackbox.emplace_with_duplicates(player, a);
					Mix_PlayChannel(-1, attack_sound, 0);
				case Direction::UP:
					a = initAB(vec2(motion.position.x, motion.position.y - 48), vec2(64.f), player_data.weapon_stat, true);
					registry.attackbox.emplace_with_duplicates(player, a);
					Mix_PlayChannel(-1, attack_sound, 0);
				case Direction::LEFT:
					a = initAB(vec2(motion.position.x - 48, motion.position.y), vec2(64.f), player_data.weapon_stat, true);
					registry.attackbox.emplace_with_duplicates(player, a);
					Mix_PlayChannel(-1, attack_sound, 0);
				case Direction::RIGHT:
					a = initAB(vec2(motion.position.x + 48, motion.position.y), vec2(64.f), player_data.weapon_stat, true);
					registry.attackbox.emplace_with_duplicates(player, a);
					Mix_PlayChannel(-1, attack_sound, 0);
				}
			}
			return;
		}
	}
		// if changed to keyboard button (working while walking too)
		if (key == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
			if (player_data.current_stamina >= 15.0f &&
				animation.current_state != AnimationState::ATTACK &&
				animation.current_state != AnimationState::BLOCK) {
				animation.setState(AnimationState::BLOCK, animation.current_dir);
				player_data.current_stamina -= 15.0f;
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
			//	renderer->show_capture_ui = true;
			}
			else {
				printf("Inventory closed.\n");
			//	renderer->show_capture_ui = false;
			}
			break;

		case GLFW_KEY_E:
			if (pickup_allowed && pickup_entity != Entity{}) {
				Inventory& inventory = registry.players.get(player).inventory;
				

				// Play the pickup sound
				Mix_PlayChannel(-1, key_sound, 0);

				// Check if the item picked up is the key
				if (pickup_item_name == "CompanionRobot") {
					Robot& robot = registry.robots.get(pickup_entity);
					inventory.addCompanionRobot(
						"CompanionRobot",
						robot.current_health,
						robot.attack,
						robot.speed
					);
				}
				else {

					inventory.addItem(pickup_item_name, 1);
				}
				if (pickup_item_name == "Key") {
					key_collected = true;  // Mark key as collected
				}
				// Remove the picked-up entity from the world
				registry.remove_all_components_of(pickup_entity);

				// Reset pickup flags
				pickup_allowed = false;
				pickup_entity = Entity{};
				pickup_item_name.clear();
			}
			break;
		case GLFW_KEY_ESCAPE:
			if (action == GLFW_PRESS) {
				// Toggle the paused state
				game_paused = !game_paused;
				renderer->game_paused = game_paused;

				if (game_paused) {
					printf("Game paused.\n");
				}
				else {
					printf("Game resumed.\n");
				}
			}
			break;
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
				player_data.current_stamina -= 10.f;
				if (player_data.current_stamina < 0.f) {
					player_data.current_stamina = 0.f;
				}
				printf("Player health: %f\n", player_data.current_stamina);
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

	if (action == GLFW_PRESS) {

		if (current_level >= 3) {

			if (key == GLFW_KEY_SPACE) {
				if (player_data.current_stamina >= 30.0f &&
					animation.current_state != AnimationState::ATTACK &&
					animation.current_state != AnimationState::BLOCK) {

					animation.setState(AnimationState::SECOND, animation.current_dir);
					player_data.current_stamina -= 30.0f;

					std::vector<std::pair<vec2, Direction>> attacks = {
						{{0, 48}, Direction::DOWN},
						{{0, -48}, Direction::UP},
						{{-48, 0}, Direction::LEFT},
						{{48, 0}, Direction::RIGHT}
					};

					for (const auto& attack : attacks) {
						attackBox a = initAB(
							motion.position + attack.first,
							vec2(64.f),
							static_cast<int>(player_data.weapon_stat * 0.8f),
							true
						);
						registry.attackbox.emplace_with_duplicates(player, a);
					}

				}
			}
		}

		if (current_level >= 4) {
			if (key == GLFW_KEY_T) {
				if (player_data.current_stamina >= 20.0f &&
					animation.current_state != AnimationState::ATTACK &&
					animation.current_state != AnimationState::BLOCK) {

					animation.setState(AnimationState::PROJ, animation.current_dir);
					player_data.current_stamina -= 20.0f;

					vec2 proj_dir;
					switch (animation.current_dir) {
					case Direction::DOWN: proj_dir = { 0, 1 }; break;
					case Direction::UP: proj_dir = { 0, -1 }; break;
					case Direction::LEFT: proj_dir = { -1, 0 }; break;
					case Direction::RIGHT: proj_dir = { 1, 0 }; break;
					}

					vec2 proj_speed = normalize(proj_dir) * 300.f;
					float angle = atan2(proj_dir.y, proj_dir.x);

					Entity proj = createProjectile(motion.position, proj_speed, angle, false, true);

				}
			}
		}
	}


	// Resetting game
	/*if (action == GLFW_RELEASE && key == GLFW_KEY_R) {
		int w, h;
		glfwGetWindowSize(window, &w, &h);

		restart_game();
	}*/

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
	
	if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
		int selectedSlotIndex = inventory.getSelectedSlot(); 
		InventorySlot& selectedSlot = inventory.slots[selectedSlotIndex]; 

		if (!selectedSlot.item.name.empty()) {
			useSelectedItem();
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

void WorldSystem::useSelectedItem() {
	int slot = playerInventory->getSelectedSlot();
	if (slot < 0 || slot >= static_cast<int>(playerInventory->slots.size())) {
		printf("Invalid slot selected.\n");
		return;
	}

	Item& selectedItem = playerInventory->slots[slot].item;
	if (selectedItem.name.empty()) {
		printf("No item in the selected slot.\n");
		return;
	}
	if (selectedItem.name == "Key") {
		for (Entity door_entity : registry.doors.entities) {
			Door& door = registry.doors.get(door_entity);

			if (door.in_range && door.is_locked) {
				auto& door_anim = registry.doorAnimations.get(door_entity);
				door_anim.is_opening = true; 
				door.is_locked = false; 
				playerInventory->removeItem(selectedItem.name, 1);
			//	printf("removing key");
				if (playerInventory->slots[slot].item.name.empty() && slot < playerInventory->slots.size() - 1) {
					playerInventory->setSelectedSlot(slot);
				}
				return;
			}
		}

		printf("No door in range to use key on.\n");
		return;
	}

	else if (selectedItem.name == "CompanionRobot") {
		vec2 placementPosition = getPlayerPlacementPosition();
		createCompanionRobot(renderer, placementPosition, selectedItem);

		playerInventory->removeItem(selectedItem.name, 1);

		if (playerInventory->slots[slot].item.name.empty() && slot < playerInventory->slots.size() - 1) {
			playerInventory->setSelectedSlot(slot);
		}
	}
	else if (selectedItem.name == "ArmorPlate") {
		Entity player_e = registry.players.entities[0];
		Player& player = registry.players.get(player_e);
		player.armor_stat += 15.0f;
	}
	else if (selectedItem.name == "IceRobot") {
		vec2 placementPosition = getPlayerPlacementPosition();
		createCompanionIceRobot(renderer, placementPosition, selectedItem);

		playerInventory->removeItem(selectedItem.name, 1);

		if (playerInventory->slots[slot].item.name.empty() && slot < playerInventory->slots.size() - 1) {
			playerInventory->setSelectedSlot(slot);
		}
	}
	else if (selectedItem.name == "HealthPotion") {
		Entity player_e = registry.players.entities[0];
		Player& player = registry.players.get(player_e);
		if (player.current_health < 100.f) {
			player.current_health += 30.f;
			if (player.current_health > player.max_health) {
				player.current_health = player.max_health;
			}

			playerInventory->removeItem(selectedItem.name, 1);

			if (playerInventory->slots[slot].item.name.empty() && slot < playerInventory->slots.size() - 1) {
				playerInventory->setSelectedSlot(slot);
			}
		}
		else {
			return;
		}
	}
	else if (selectedItem.name == "Teleporter") {
		Motion& player_motion = registry.motions.get(player);
		float edge_proximity = 64.0f; // needs some work
		float map_width_px = map_width * 64; 
		float map_height_px = map_height * 64;

		if (player_motion.position.x < edge_proximity ||
			player_motion.position.x > map_width_px - edge_proximity ||
			player_motion.position.y < edge_proximity ||
			player_motion.position.y > map_height_px - edge_proximity) {
			printf("Cannot use Teleporter near map edges.\n");
			return;
		}

		Entity player_e = registry.players.entities[0];
		Player& player = registry.players.get(player_e);

		if (!player.isDashing && player.dashCooldown <= 0.f) {
			player.isDashing = true;
			player.dashTimer = 0.7f; 
			player.dashCooldown = 2.0f; 
			vec2 dashDirection = normalize(registry.motions.get(player_e).target_velocity);
			if (glm::length(dashDirection) == 0) {
				dashDirection = vec2(1.f, 0.f); 
			}
			player.lastDashDirection = dashDirection;
		}

		playerInventory->removeItem(selectedItem.name, 1);
		if (playerInventory->slots[slot].item.name.empty() && slot < playerInventory->slots.size() - 1) {
			playerInventory->setSelectedSlot(slot);
		}
	}


	else if (selectedItem.name == "Energy Core") {
		Entity player_e = registry.players.entities[0];
		Player& player = registry.players.get(player_e);
		player.max_stamina += 5.f; 
		player.current_stamina = std::min(player.current_stamina + 20.f, player.max_stamina);

		playerInventory->removeItem(selectedItem.name, 1);
		
		if (playerInventory->slots[slot].item.name.empty() && slot < playerInventory->slots.size() - 1) {
			playerInventory->setSelectedSlot(slot);
		}
	}

}




vec2 WorldSystem::getPlayerPlacementPosition() {
	Entity playerEntity = registry.players.entities[0];
	Motion& playerMotion = registry.motions.get(playerEntity);
	return playerMotion.position;
}
void WorldSystem::on_mouse_move(glm::vec2 position) {
	renderer->mousePosition = position;
}
void WorldSystem::onMouseClickCaptureUI(int button, int action, int mods) {
	vec2 c_button_position = vec2(850.f, 410.f);
	vec2 c_button_size = vec2(100.f, 100.f);
	vec2 d_button_position = vec2(375.f, 410.f);
	vec2 d_button_size = vec2(100.f, 100.f);
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			if (renderer->mousePosition.x >= c_button_position.x &&
				renderer->mousePosition.x <= c_button_position.x + c_button_size.x &&
				renderer->mousePosition.y >= c_button_position.y &&
				renderer->mousePosition.y <= c_button_position.y + c_button_size.y) {

				handleCaptureButtonClick();  // Call the Capture handler
				return;
			}

			if (renderer->mousePosition.x >= d_button_position.x &&
				renderer->mousePosition.x <= d_button_position.x + d_button_size.x &&
				renderer->mousePosition.y >= d_button_position.y &&
				renderer->mousePosition.y <= d_button_position.y + d_button_size.y) {

				handleDisassembleButtonClick();  // Call the Disassemble handler
				return;
			}
		}
	}
}
void WorldSystem::onMouseClick(int button, int action, int mods) {
    vec2 upgrade_button_position = vec2(730.f, 310.f);
    vec2 upgrade_button_size = vec2(100.f, 100.f);
	// Handle release over armor slot
	vec2 armor_slot_position = vec2(620.f, 165.f);
	vec2 armor_slot_size = vec2(90.f, 90.f);
	vec2 weapon_slot_position = vec2(620.f, 260.f);
	vec2 weapon_slot_size = vec2(90.f, 90.f);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            renderer->mouseReleased = false;
		

            if (renderer->mousePosition.x >= armor_slot_position.x &&
                renderer->mousePosition.x <= armor_slot_position.x + armor_slot_size.x &&
                renderer->mousePosition.y >= armor_slot_position.y &&
                renderer->mousePosition.y <= armor_slot_position.y + armor_slot_size.y) {

                if (!playerInventory->slots[10].item.name.empty()) {
                    renderer->isDragging = true;
                    renderer->draggedSlot = 10;
                    renderer->dragOffset = renderer->mousePosition - armor_slot_position;
                    return;
                }
            }


            if (renderer->mousePosition.x >= weapon_slot_position.x &&
                renderer->mousePosition.x <= weapon_slot_position.x + weapon_slot_size.x &&
                renderer->mousePosition.y >= weapon_slot_position.y &&
                renderer->mousePosition.y <= weapon_slot_position.y + weapon_slot_size.y) {

                if (!playerInventory->slots[11].item.name.empty()) {
                    renderer->isDragging = true;
                    renderer->draggedSlot = 11;
                    renderer->dragOffset = renderer->mousePosition - weapon_slot_position;
                    return;
                }
            }

            // Check if mouse is pressed on upgrade button
            if (renderer->mousePosition.x >= upgrade_button_position.x &&
                renderer->mousePosition.x <= upgrade_button_position.x + upgrade_button_size.x &&
                renderer->mousePosition.y >= upgrade_button_position.y &&
                renderer->mousePosition.y <= upgrade_button_position.y + upgrade_button_size.y) {

                handleUpgradeButtonClick();
                return;
            }

            // Check normal slots if not armor/weapon
            for (int i = 0; i < 10; ++i) {
                vec2 slotPosition = renderer->getSlotPosition(i);
                vec2 slotSize = vec2(90.f, 90.f);

                if (renderer->mousePosition.x >= slotPosition.x &&
                    renderer->mousePosition.x <= slotPosition.x + slotSize.x &&
                    renderer->mousePosition.y >= slotPosition.y &&
                    renderer->mousePosition.y <= slotPosition.y + slotSize.y) {

                    if (!playerInventory->slots[i].item.name.empty()) {
                        renderer->isDragging = true;
                        renderer->draggedSlot = i;
                        renderer->dragOffset = renderer->mousePosition - slotPosition;
                    }
                    break;
                }
            }
        }
        else if (action == GLFW_RELEASE && renderer->isDragging) {
            renderer->mouseReleased = true;

   
            if (renderer->mousePosition.x >= armor_slot_position.x &&
                renderer->mousePosition.x <= armor_slot_position.x + armor_slot_size.x &&
                renderer->mousePosition.y >= armor_slot_position.y &&
                renderer->mousePosition.y <= armor_slot_position.y + armor_slot_size.y) {

                if (!playerInventory->slots[10].item.name.empty()) {
                    bool placedInNormalSlot = false;
                    for (int i = 0; i < 10; ++i) {
                        if (playerInventory->slots[i].item.name.empty()) {
                            playerInventory->slots[i].item = playerInventory->slots[10].item;
                            placedInNormalSlot = true;
                            break;
                        }
                    }
                    if (!placedInNormalSlot) {
                        std::swap(playerInventory->slots[renderer->draggedSlot].item, playerInventory->slots[10].item);
                    }
                }
                playerInventory->slots[10].item = playerInventory->slots[renderer->draggedSlot].item;
                playerInventory->slots[renderer->draggedSlot].item = {};
            }
            // Check if releasing over the weapon slot
            else if (renderer->mousePosition.x >= weapon_slot_position.x &&
                     renderer->mousePosition.x <= weapon_slot_position.x + weapon_slot_size.x &&
                     renderer->mousePosition.y >= weapon_slot_position.y &&
                     renderer->mousePosition.y <= weapon_slot_position.y + weapon_slot_size.y) {
				printf("WEAPON SLOT");
                if (!playerInventory->slots[11].item.name.empty()) {
                    bool placedInNormalSlot = false;
                    for (int i = 0; i < 10; ++i) {
                        if (playerInventory->slots[i].item.name.empty()) {
                            playerInventory->slots[i].item = playerInventory->slots[11].item;
                            placedInNormalSlot = true;
                            break;
                        }
                    }
                    if (!placedInNormalSlot) {
                        std::swap(playerInventory->slots[renderer->draggedSlot].item, playerInventory->slots[11].item);
                    }
                }
                playerInventory->slots[11].item = playerInventory->slots[renderer->draggedSlot].item;
                playerInventory->slots[renderer->draggedSlot].item = {};
            }
            else {
                // Normal slot handling
                for (int i = 0; i < 10; ++i) {
                    vec2 targetSlotPosition = renderer->getSlotPosition(i);
                    vec2 slotSize = vec2(90.f, 90.f);

                    if (renderer->mousePosition.x >= targetSlotPosition.x &&
                        renderer->mousePosition.x <= targetSlotPosition.x + slotSize.x &&
                        renderer->mousePosition.y >= targetSlotPosition.y &&
                        renderer->mousePosition.y <= targetSlotPosition.y + slotSize.y) {

                        if (playerInventory->slots[i].item.name.empty()) {
                            playerInventory->slots[i].item = playerInventory->slots[renderer->draggedSlot].item;
                            playerInventory->slots[renderer->draggedSlot].item = {};
                        }
                        else {
                            std::swap(playerInventory->slots[renderer->draggedSlot].item, playerInventory->slots[i].item);
                        }
                        break;
                    }
                }
            }

            renderer->isDragging = false;
            renderer->draggedSlot = -1;
        }
    }
}
void WorldSystem::handleCaptureButtonClick() {
	
	//int health = 50;   
	//int damage = 25;   
	//int speed = 10;  

	// Perform the capture logic
	printf("Robot captured successfully!\n");

	// Add the captured robot to the inventory as a companion
	Robot& robot = registry.robots.get(renderer->currentRobotEntity);
	
	if   (registry.iceRobotAnimations.has(renderer->currentRobotEntity)) {
		printf("Hello, World!\n");
		playerInventory->addCompanionRobot("IceRobot", robot.current_health, robot.attack, robot.speed);
		renderer->show_capture_ui = false;
		robot.showCaptureUI = false;
		uiScreenShown = false;
		registry.remove_all_components_of(renderer->currentRobotEntity);
	} else if (registry.robots.has(renderer->currentRobotEntity)) {
		std::string robotName = "CompanionRobot";
		playerInventory->addCompanionRobot(robotName, robot.current_health, robot.attack, robot.speed);
		renderer->show_capture_ui = false;
		robot.showCaptureUI = false;
		uiScreenShown = false;
		registry.remove_all_components_of(renderer->currentRobotEntity);


	}
}

void WorldSystem::handleDisassembleButtonClick() {
	if (!renderer->currentRobotEntity) {
		printf("No robot selected for disassembly!\n");
		return;
	}

	Robot& robot = registry.robots.get(renderer->currentRobotEntity);

	if (robot.disassembleItems.empty()) {
		printf("No items available to disassemble!\n");
		return;
	}

	for (const Item& item : robot.disassembleItems) {
		playerInventory->addItem(item.name, item.quantity);
		printf("Added %d x %s to inventory.\n", item.quantity, item.name.c_str());
	}

	uiScreenShown = false;
	renderer->show_capture_ui = false;
	registry.remove_all_components_of(renderer->currentRobotEntity); 
	renderer->currentRobotEntity = Entity();

}



void WorldSystem::handleUpgradeButtonClick() {
	Player& player_data = registry.players.get(player);
	auto& inventory_slots = player_data.inventory.slots;

	const int armor_slot_index = 10;
	const int weapon_slot_index = 11;

	if ((inventory_slots[armor_slot_index].item.name == "CompanionRobot" ||
		inventory_slots[armor_slot_index].item.name == "IceRobot") &&
		inventory_slots[armor_slot_index].item.quantity > 0) {

		int total_upgrade = 0;

		for (auto& slot : inventory_slots) {
			if ((slot.item.name == "CompanionRobot" || slot.item.name == "IceRobot") && slot.item.quantity > 0) {
				total_upgrade += 10 * slot.item.quantity;
			}
		}

		if (inventory_slots[weapon_slot_index].item.name == "Robot Parts" && inventory_slots[weapon_slot_index].item.quantity > 0) {
			Item& equipped_robot = inventory_slots[armor_slot_index].item;

			equipped_robot.speed = static_cast<int>(equipped_robot.speed * 1.15f);
			equipped_robot.health = static_cast<int>(equipped_robot.health * 1.15f);
			equipped_robot.damage = static_cast<int>(equipped_robot.damage * 1.15f);

			std::cout << equipped_robot.name << " in the armor slot upgraded: +5% to speed, health, and damage!" << std::endl;

			player_data.inventory.removeItem("Robot Parts", 1);
		}
	}
	else {
		std::cout << "No valid robot (CompanionRobot or IceRobot) equipped in the armor slot. Upgrade not performed." << std::endl;
	}
}






void WorldSystem::load_level(int level) {
	for (auto entity : registry.motions.entities) {
		if (entity != player) registry.remove_all_components_of(entity);
	}
	/*registry.tilesets.clear();
	registry.tiles.clear();*/
	ScreenState& screen = registry.screenStates.components[0];
	// Level-specific setup
	switch (level) {
	case 1:
		//registry.maps.clear();
		map_width = 21;
		map_height = 18;
		printf("loading remote level");
		screen.is_nighttime = true;
		load_remote_location(21, 18);
		break;
	case 2:
		// Setup for Level 2
		map_width = 40;
		map_height = 27;
		printf("map_height: %d" + map_height);
		printf("map_width: %d" + map_width);
		registry.maps.clear();
		screen.is_nighttime = false;
		renderer->show_capture_ui = false;
		load_first_level(40, 27);
		//generate_json(registry);
		break;
	case 3:
		// Setup for Level 3
		registry.maps.clear();
		map_width = 40;
		map_height = 28;
		//screen.is_nighttime = false;
		renderer->show_capture_ui = false;
		load_second_level(40, 28);
		//generate_json(registry);
		break;
	case 4:
		// Setup for Level 3
		registry.maps.clear();
		map_width = 64;
		map_height = 40;
		screen.is_nighttime = true;
		renderer->show_capture_ui = false;
		load_boss_level(64, 40);
		//generate_json(registry);
		break;
	default:
		return;
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

void WorldSystem::initializeCutscene() {
	Entity cutscene_entity = registry.cutscenes.entities[0];
	Cutscene& cutscene = registry.cutscenes.emplace(cutscene_entity);

	cutscene.is_active = true;
	cutscene.duration = 10.0f;
	cutscene.camera_control_enabled = true;

	cutscene.actions.push_back([&](float elapsed_time) {
		if (elapsed_time < 2.0f) {
			cutscene.camera_target_position = vec2(100, 100);
		}
		else if (elapsed_time >= 2.0f && elapsed_time < 5.0f) {
			cutscene.camera_target_position = vec2(200, 300);
		}
		else if (elapsed_time >= 5.0f) {
			cutscene.camera_target_position = vec2(400, 400);
		}
		});
}


void WorldSystem::updateCutscenes(float elapsed_ms) {
	for (Entity entity : registry.cutscenes.entities) {
		Cutscene& cutscene = registry.cutscenes.get(entity);

		if (cutscene.is_active) {
			cutscene.current_time += elapsed_ms / 1000.0f;

			for (auto& action : cutscene.actions) {
				action(cutscene.current_time);
			}

			if (cutscene.camera_control_enabled) {
				renderer->updateCameraPosition(cutscene.camera_target_position);
			}

			if (cutscene.current_time >= cutscene.duration) {
				cutscene.is_active = false;
				enablePlayerControl();
			}
		}
	}
}

void WorldSystem::disablePlayerControl() {
	player_control_enabled = false;
	std::cout << "Player control disabled" << std::endl;
}

void WorldSystem::enablePlayerControl() {
	player_control_enabled = true;
	std::cout << "Player control enabled" << std::endl;
}


void to_json(json& j, const WorldSystem& ws) {
	j = json{
		{"current_level", ws.get_current_level()},
		{"player", ws.get_player()},
		{"spaceship", ws.get_spaceship()}
	};
}
void from_json(const json& j, WorldSystem& ws) {
	ws.set_current_level(j["current_level"]);
	ws.set_player(j["player"]);
	ws.set_spaceship(j["spaceship"]);
}
