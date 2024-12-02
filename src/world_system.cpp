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
#include <unordered_set>
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
	, playerInventory(nullptr)
	, ai_system(){
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
	if (door_open != nullptr)
		Mix_FreeChunk(door_open);
	if (robot_attack != nullptr)
		Mix_FreeChunk(robot_attack);
	if (robot_ready_attack != nullptr)
		Mix_FreeChunk(robot_ready_attack);
	if (robot_death != nullptr)
		Mix_FreeChunk(robot_death);
	if (robot_awake != nullptr)
		Mix_FreeChunk(robot_awake);
	if (Upgrade != nullptr)
		Mix_FreeChunk(Upgrade);
	if (teleport_sound != nullptr)
		Mix_FreeChunk(teleport_sound);
	if (using_item != nullptr)
		Mix_FreeChunk(using_item);
	if (insert_card != nullptr)
		Mix_FreeChunk(insert_card);

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
	door_open = Mix_LoadWAV(audio_path("door_open.wav").c_str());
	robot_attack = Mix_LoadWAV(audio_path("robot_attack.wav").c_str());
	robot_ready_attack = Mix_LoadWAV(audio_path("robot_ready_attack.wav").c_str());
	robot_death = Mix_LoadWAV(audio_path("robot_death.wav").c_str());
	robot_awake = Mix_LoadWAV(audio_path("robot_awake.wav").c_str());
	Upgrade = Mix_LoadWAV(audio_path("Upgrade.wav").c_str());
	teleport_sound = Mix_LoadWAV(audio_path("teleport_sound.wav").c_str());
	using_item = Mix_LoadWAV(audio_path("using_item.wav").c_str());
	insert_card = Mix_LoadWAV(audio_path("insert_card.wav").c_str());


	if (background_music == nullptr || player_dead_sound == nullptr || key_sound == nullptr || collision_sound == nullptr || attack_sound == nullptr 
		|| door_open == nullptr || robot_attack == nullptr || robot_ready_attack == nullptr || robot_death == nullptr || Upgrade == nullptr
		|| robot_awake == nullptr || teleport_sound == nullptr || using_item == nullptr || insert_card == nullptr) {
		fprintf(stderr, "Failed to load sounds\n %s\n %s\n %s\n make sure the data directory is present",
			audio_path("Galactic.wav").c_str(),
			audio_path("death_hq.wav").c_str(),
			audio_path("win.wav").c_str(),
			audio_path("wall_contact.wav").c_str(),
			audio_path("attack_sound.wav").c_str(),
			audio_path("armor_break.wav").c_str(),
			audio_path("door_open.wav").c_str(),
			audio_path("robot_attack.wav").c_str(),
			audio_path("robot_ready_attack.wav").c_str(),
			audio_path("robot_death.wav").c_str(),
			audio_path("robot_awake.wav").c_str(),
			audio_path("Upgrade.wav").c_str(),
			audio_path("teleport_sound.wav").c_str(),
			audio_path("using_item.wav").c_str(),
			audio_path("insert_card.wav").c_str()
		);
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

void WorldSystem::play_attack_sound() {
	if (robot_attack) {
		Mix_PlayChannel(-1, robot_attack, 0);
	}
}

void WorldSystem::play_ready_attack_sound() {
	if (robot_ready_attack) {
		Mix_PlayChannel(-1, robot_ready_attack, 0);
	}
}

void WorldSystem::play_death_sound() {
	if (robot_death) {
		Mix_PlayChannel(-1, robot_death, 0);
	}
}

void WorldSystem::play_awake_sound() {
	if (robot_awake) {
		Mix_PlayChannel(-1, robot_awake, 0);
	}
}

float lerp(float a, float b, float t) {
	return a + t * (b - a);
}

glm::vec3 lerp_color(glm::vec3 a, glm::vec3 b, float t) {
	return glm::vec3(lerp(a.x, b.x, t), lerp(a.y, b.y, t), lerp(a.z, b.z, t));
}

void WorldSystem::spawnBatSwarm(vec2 center, int count) {
	float radius = 100.f;
	for (int i = 0; i < count; i++) {
		float angle = (2.f * M_PI * i) / count;
		vec2 offset = vec2(cos(angle), sin(angle)) * radius;
		createBat(renderer, center + offset);
	}
}

void WorldSystem::updateDoorAnimations(float elapsed_ms) {
	for (auto entity : registry.doors.entities) {
		Door& door = registry.doors.get(entity);
		DoorAnimation& animation = registry.doorAnimations.get(entity);

		if (animation.is_opening && !door.is_open) {
			animation.update(elapsed_ms);
			// add door_open sound
			Mix_PlayChannel(-1, door_open, 0);

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
bool WorldSystem::hasPlayerMoved() {
	Motion& motion = registry.motions.get(player);
	return motion.target_velocity.x != 0 || motion.target_velocity.y != 0;
}


bool WorldSystem::playerHasAttacked() {
	Player& player_data = registry.players.get(player);
	return registry.attackbox.has(player);
}

bool WorldSystem::playerNearKey() {
	for (Entity entity : registry.keys.entities) {
		Motion& armorMotion = registry.motions.get(entity);
		Motion& playerMotion = registry.motions.get(player);
		float distance = glm::length(playerMotion.position - armorMotion.position);
		if (distance < 64.0f) { // Define `INTERACTION_RADIUS` as a constant
			return true;
		}
	}
	return false;
}
bool WorldSystem::playerNearArmor() {
	for (Entity entity : registry.armorplates.entities) {
		Motion& armorMotion = registry.motions.get(entity);
		Motion& playerMotion = registry.motions.get(player);
		float distance = glm::length(playerMotion.position - armorMotion.position);
		if (distance < 64.0f) { // Define `INTERACTION_RADIUS` as a constant
			return true;
		}
	}
	return false;
}
bool WorldSystem::playerNearPotion() {
	for (Entity entity : registry.potions.entities) {
		Motion& armorMotion = registry.motions.get(entity);
		Motion& playerMotion = registry.motions.get(player);
		float distance = glm::length(playerMotion.position - armorMotion.position);
		if (distance < 64.0f) {
			return true;
		}
	}
	return false;
}
bool WorldSystem::playerPickedUpArmor() {
	return playerInventory && playerInventory->containsItem("ArmorPlate");
}

bool WorldSystem::playerUsedArmor() {
	Player& player_p = registry.players.get(player);
	return playerInventory && player_p.armor_stat > 0; // Check if armor is equipped
}


bool WorldSystem::isKeyAllowed(int key) const {
	static std::unordered_set<int> allowedKeys;
	static TutorialState lastState = TutorialState::INTRO;
	if (tutorial_state != lastState || tutorial_state == TutorialState::INTRO) {
		allowedKeys.clear(); // Clear the set on state change
		switch (tutorial_state) {
		case TutorialState::INTRO:
			allowedKeys.insert(GLFW_KEY_ENTER); // Allow ENTER during INTRO
			break;

		case TutorialState::MOVEMENT:
			allowedKeys.insert(GLFW_KEY_ENTER);
			allowedKeys.insert({ GLFW_KEY_W, GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D });
			break;

		case TutorialState::EXPLORATION:
			allowedKeys.insert(GLFW_KEY_ENTER);
			allowedKeys.insert({ GLFW_KEY_W, GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D, GLFW_KEY_E, GLFW_KEY_LEFT_SHIFT });
			allowedKeys.insert({ GLFW_KEY_Q, GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3 });
			break;

		case TutorialState::ATTACK_HINT:
			allowedKeys.insert(GLFW_KEY_ENTER);
			allowedKeys.insert({ GLFW_KEY_W, GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D, GLFW_KEY_E, GLFW_KEY_LEFT_SHIFT });
			allowedKeys.insert({ GLFW_KEY_Q, GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_MOUSE_BUTTON_LEFT });
			break;

		case TutorialState::COMPLETED:
			return true;

		default:
			break;
		}
		lastState = tutorial_state;
	}

	// Check if the key is allowed
	return allowedKeys.find(key) != allowedKeys.end();
}

void WorldSystem::updateTutorialState() {
	if (tutorial_state == TutorialState::COMPLETED) {
		return;
	}
	switch (tutorial_state) {

	case TutorialState::INTRO:
		if (!renderer->playing_cutscene) {
			if (!introNotificationsAdded) {
				notificationQueue.emplace("Ouch, that was a rough landing.", 5.0f);
				notificationQueue.emplace("Where am I? I need to get outside.", 5.0f);
				introNotificationsAdded = true;
			}

			if (notificationQueue.empty() && registry.notifications.entities.empty()) {

				tutorial_state = TutorialState::MOVEMENT;
				renderer->tutorial_state = tutorial_state;
			}
		}
		break;

	case TutorialState::MOVEMENT:
		if (!hasPlayerMoved() && !movementHintShown) {
			notificationQueue.emplace("Hint: Use WASD keys to move around.", 3.0f);
			movementHintShown = true;
		}
		else if (hasPlayerMoved()) {
			notificationQueue.emplace("I need to stock up on some resources before getting out there.", 5.0f);
			tutorial_state = TutorialState::EXPLORATION;
			renderer->tutorial_state = tutorial_state;
		}
		break;

	case TutorialState::EXPLORATION:
		if (!armorPickedUp && playerNearArmor()) {
			notificationQueue.emplace("Radiation levels outside seem to be high, I better wear some protection.", 5.0f);
			armorPickedUp = true;
			pickupHintShown = true;
		}

		if (!potionPickedUp && playerNearPotion()) {
			notificationQueue.emplace("This should come in handy.", 5.0f);
			potionPickedUp = true;
			pickupHintShown = true;
		}
		if (!keyPickedUp && playerNearKey()) {
			notificationQueue.emplace("I can use this to open the door.", 5.0f);
			pickupHintShown = true;
			keyPickedUp = true;
		}
		if ((!armorPickedUp || !potionPickedUp || !keyPickedUp) && !pickupHintShown) {
			registry.notifications.clear();
			while (!notificationQueue.empty()) {
				notificationQueue.pop();
			}
			notificationQueue.emplace("Hint: Press [E] to pick up items.", 3.0f);
			pickupHintShown = true;
		}
		playerUsedArmor();

		if (armorPickedUp && potionPickedUp && keyPickedUp) {
			registry.notifications.clear();
			notificationQueue.emplace("Hint: Switch inventory slots using 123 and [Q] to use the item.", 5.0f);
		}/*
		printf("current_level %d", current_level);*/
		if (current_level == 1) {
			/*printf("LEVEL 1");*/
			registry.notifications.clear();
			while (!notificationQueue.empty()) {
				notificationQueue.pop();
			}

			attackNotificationsAdded = false;
			tutorial_state = TutorialState::LEAVE_SPACESHIP_HINT;
			renderer->tutorial_state = tutorial_state;
		}
		break;
	case TutorialState::LEAVE_SPACESHIP_HINT:
		if (!attackNotificationsAdded) {
			while (!notificationQueue.empty()) {
				notificationQueue.pop();
			}
			notificationQueue.emplace("Oh no, these guys don't seem friendly.", 3.0f);
			notificationQueue.emplace("Hint: Left click to attack", 3.0f);
			attackNotificationsAdded = true;
		}

		if (notificationQueue.empty() && registry.notifications.entities.empty() && attackNotificationsAdded) {
			attackNotificationsAdded = false;
			tutorial_state = TutorialState::ATTACK_HINT;
			renderer->tutorial_state = tutorial_state;
		}
		break;

	case TutorialState::ATTACK_HINT:
		//	if (playerHasAttacked()) {
	
			for (auto& slot : playerInventory->slots) {
				if (slot.item.name == "Robot Parts") {
					robotPartsCount += slot.item.quantity;
					if (robotPartsCount >= 5) {
						tutorial_state = TutorialState::ROBOT_PARTS_HINT;
						renderer->tutorial_state = tutorial_state;
						break; // Exit the loop early
					}
				}

			}

		if (!sprintHintShown) {
			notificationQueue.emplace("Hint: Hold [Left Shift] to sprint.", 3.0f);
			sprintHintShown = true;
		}
		// check if player has 5 robot parts in inventory. if so move to ROBOT_PARTS_HINT.
		break;

	case TutorialState::ROBOT_PARTS_HINT:
		if (!attackNotificationsAdded) {
			notificationQueue.emplace("Robot parts acquired!", 3.0f);
			notificationQueue.emplace("These items can be used to upgrade other robots you capture.", 5.0f);
			notificationQueue.emplace("Hint: Press [I] to open and close your inventory.", 5.0f);
		
			attackNotificationsAdded = true;
			inventoryOpened = false;
			inventoryClosed = false;
		}
		if (inventoryOpened && inventoryClosed) {
			registry.notifications.clear();
			while (!notificationQueue.empty()) {
				notificationQueue.pop();
			}
			printf("Inventory interaction complete.\n");
			notificationQueue.emplace("I think I'm ready to head in now.", 5.0f);
			tutorial_state = TutorialState::COMPLETED; // Transition to the next state
			renderer->tutorial_state = tutorial_state;
		}
		break;

	case TutorialState::COMPLETED:
		// Tutorial is complete
		break;

	default:
		break;
	}

}



bool WorldSystem::step(float elapsed_ms_since_last_update) {
	if (renderer->show_start_screen) {
		return true;
	}

	//renderer->updateCutscene(elapsed_ms_since_last_update);
	if (renderer->playing_cutscene) {
		renderer->cutscene_timer += elapsed_ms_since_last_update / 1000.f;
		if (renderer->cutscene_timer >= renderer->cutscene_duration_per_image) {
			renderer->cutscene_timer = 0.f; // Reset timer
			renderer->current_cutscene_index++;
			if (renderer->current_cutscene_index >= renderer->cutscene_images.size()) {
				// End cutscene when all images are shown
				renderer->playing_cutscene = false;
				renderer->current_cutscene_index = 0;
				//renderer->cutscene_images.clear();
				renderer->skipCutscene();
				std::cout << "Cutscene ended." << std::endl;
			}
		}
	}


	if (registry.players.has(player)) {
		playerInventory = &registry.players.get(player).inventory;
	}


	//if (current_level == 0) {
	updateNotifications(elapsed_ms_since_last_update);
	updateTutorialState();
	//}
	for (auto entity : registry.robots.entities) {
		Robot& robot = registry.robots.get(entity);
		if (robot.showCaptureUI) {
			uiScreenShown = true;
			break;
		}
	}
	while (registry.debugComponents.entities.size() > 0)
		registry.remove_all_components_of(registry.debugComponents.entities.back());

	ScreenState& screen = registry.screenStates.components[0];

	if (screen.fade_in_progress) {
		screen.fade_in_factor -= elapsed_ms_since_last_update / 3000.f;
		if (screen.fade_in_factor <= 0.f) {
			screen.fade_in_factor = 0.f;
			screen.fade_in_progress = false;
		}
	}
	Player& p = registry.players.get(player);
	if (is_sprinting) {

		if (p.current_stamina > 0.f) {
			float stamina_loss = 5.0f * elapsed_ms_since_last_update / 1000.f;
			p.current_stamina = std::max(0.f, p.current_stamina - stamina_loss);
		}
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
	}
	else {
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
		(current_level == 4 && player_motion.position.y >= map_height_px - 64) ||
		(current_level != 3 && player_motion.position.x >= map_width_px - 64)) {
		std::cout << "Current level: " << current_level << std::endl;

		if (current_level == 0) {
			if ((tutorial_state == TutorialState::EXPLORATION  && playerUsedArmor()) || tutorial_state == TutorialState::COMPLETED) {
				current_level++;
				//				registry.notifications.clear();
				load_level(current_level);
				key_collected = false;
			}
			else {
				registry.notifications.clear();
				notificationQueue.emplace("You must wear protection before going outside.", 5.0f);
				//	createNotification("You must wear protection before going outside.", 3.0f);
			}
		}
		else if (current_level == 1) {
			if ((tutorial_state == TutorialState::COMPLETED)) {
				current_level++;
				//registry.notifications.clear();
				load_level(current_level);
				key_collected = false;
			}
			else {

				registry.notifications.clear();
				notificationQueue.emplace("You must complete the tutorial first", 5.0f);
				//createNotification("You must complete the tutorial first.", 3.0f);
			}
		}
		else {
			current_level++;
			registry.notifications.clear();
			while (!notificationQueue.empty()) {
				notificationQueue.pop();
			}
			load_level(current_level);
			key_collected = false; // Reset key_collected for the next level, if required
		}
	}

	if (p.armor_stat == 0) {
		Entity radiation_entity = *registry.radiations.entities.begin();
		Radiation& radiation_data = registry.radiations.get(radiation_entity);
		float health_loss = radiation_data.damagePerSecond * elapsed_ms_since_last_update / 6000.f;
		p.current_health = std::max(0.f, p.current_health - health_loss);
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

	ai_system.step(elapsed_ms_since_last_update);

	
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
void WorldSystem::updateNotifications(float elapsed_ms) {
	static float notification_timer = 0.f;

	if (!registry.notifications.entities.empty()) {
		Entity activeNotification = registry.notifications.entities[0];

		if (registry.notifications.has(activeNotification)) {
			Notification& notification = registry.notifications.get(activeNotification);

			notification_timer += elapsed_ms / 1000.f;

			if (notification_timer >= notification.duration) {
				registry.notifications.remove(activeNotification);

				notification_timer = 0.f;
			}
		}
	}
	else if (!notificationQueue.empty()) {
		auto nextNotification = notificationQueue.front();
		notificationQueue.pop();

		createNotification(nextNotification.first, nextNotification.second);
	}
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
	new_tileset_component.tileset.initializeTileTextureMap(7, 52);  // Initialize with new tileset

	// Load the new grass and obstacle maps for the new scene
	std::vector<std::vector<int>> new_grass_map = new_tileset_component.tileset.initializeSecondLevelMap();
	obstacle_map = new_tileset_component.tileset.initializeSecondLevelObstacleMap();


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
	for (int y = 0; y < obstacle_map.size(); y++) {
		for (int x = 0; x < obstacle_map[y].size(); x++) {
			int tile_id = obstacle_map[y][x];
			if (tile_id != 0) {
				vec2 position = { x * tilesize - (tilesize / 2) + tilesize, y * tilesize - (tilesize / 2) + tilesize };
				Entity tile_entity = createTileEntity(renderer, new_tileset_component.tileset, position, tilesize, tile_id);
				Tile& tile = registry.tiles.get(tile_entity);
				tile.walkable = false;  // Mark as non-walkable
				tile.atlas = TEXTURE_ASSET_ID::TILE_ATLAS_LEVELS;  // Use the new tile atlas
			}
		}
	}

	createTile_map(obstacle_map, tilesize);

	float new_spawn_x = tilesize;
	float new_spawn_y = tilesize * 2;
	Motion& player_motion = registry.motions.get(player);
	player_motion.position = { new_spawn_x, new_spawn_y };

	createBottomDoor(renderer, { tilesize * 24, tilesize * 35 });

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

void WorldSystem::load_third_level(int map_width, int map_height) {
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
	new_tileset_component.tileset.initializeTileTextureMap(7, 52);// Initialize with new tileset

	// Load the new grass and obstacle maps for the new scene
	std::vector<std::vector<int>> new_grass_map = new_tileset_component.tileset.initializeThirdLevelMap();
	obstacle_map = new_tileset_component.tileset.initializeThirdLevelObstacleMap();


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
	for (int y = 0; y < obstacle_map.size(); y++) {
		for (int x = 0; x < obstacle_map[y].size(); x++) {
			int tile_id = obstacle_map[y][x];
			if (tile_id != 0) {
				vec2 position = { x * tilesize - (tilesize / 2) + tilesize, y * tilesize - (tilesize / 2) + tilesize };
				Entity tile_entity = createTileEntity(renderer, new_tileset_component.tileset, position, tilesize, tile_id);
				Tile& tile = registry.tiles.get(tile_entity);
				tile.walkable = false;  // Mark as non-walkable
				tile.atlas = TEXTURE_ASSET_ID::TILE_ATLAS_LEVELS;  // Use the new tile atlas
			}
		}
	}

	createTile_map(obstacle_map, tilesize);

	float new_spawn_x = tilesize * 9;
	float new_spawn_y = tilesize * 1;
	Motion& player_motion = registry.motions.get(player);
	player_motion.position = { new_spawn_x, new_spawn_y };


	//renderer->updateCameraPosition({ new_spawn_x, new_spawn_y });

	spawnBatSwarm(vec2(tilesize * 9, tilesize * 5), 15);

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
	new_tileset_component.tileset.initializeTileTextureMap(7, 52);  // Initialize with new tileset

	// Load the new grass and obstacle maps for the new scene
	std::vector<std::vector<int>> new_grass_map = new_tileset_component.tileset.initializeFinalLevelMap();
	obstacle_map = new_tileset_component.tileset.initializeFinalLevelObstacleMap();

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
	for (int y = 0; y < obstacle_map.size(); y++) {
		for (int x = 0; x < obstacle_map[y].size(); x++) {
			int tile_id = obstacle_map[y][x];
			if (tile_id != 0) {
				vec2 position = { x * tilesize - (tilesize / 2) + tilesize, y * tilesize - (tilesize / 2) + tilesize };
				Entity tile_entity = createTileEntity(renderer, new_tileset_component.tileset, position, tilesize, tile_id);
				Tile& tile = registry.tiles.get(tile_entity);
				tile.walkable = false;  // Mark as non-walkable
				tile.atlas = TEXTURE_ASSET_ID::TILE_ATLAS_LEVELS;  // Use the new tile atlas
			}
		}
	}

	createTile_map(obstacle_map, tilesize);
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
	}
	else {
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
	registry.radiations.emplace(Entity{}, 0.1f, 0.2f);
	// tutorial related stuff
	tutorial_state = TutorialState::INTRO;
	introNotificationsAdded = false;
	armorPickedUp = false;
	potionPickedUp = false;
	movementHintShown = false;
	pickupHintShown = false;
	sprintHintShown = false;
	registry.notifications.clear();
	while (!notificationQueue.empty()) {
		notificationQueue.pop();
	}
	while (registry.motions.entities.size() > 0) {
		registry.remove_all_components_of(registry.motions.entities.back());
	}

	current_level = 0;

	load_level(current_level);


}

void WorldSystem::load_first_level(int map_width, int map_height) {

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
	grass_tileset_component.tileset.initializeTileTextureMap(7, 52); // atlas size

	int tilesize = 64;

	std::vector<std::vector<int>> grass_map = grass_tileset_component.tileset.initializeFirstLevelMap();
	obstacle_map = grass_tileset_component.tileset.initializeFirstLevelObstacleMap();

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

void WorldSystem::load_tutorial_level(int map_width, int map_height) {
	auto spawn_tileset_entity = Entity();
	TileSetComponent& spawn_tileset_component = registry.tilesets.emplace(spawn_tileset_entity);
	spawn_tileset_component.tileset.initializeTileTextureMap(7, 52);

	int tilesize = 64;

	std::vector<std::vector<int>> grass_map = spawn_tileset_component.tileset.initializeTutorialLevelMap();
	obstacle_map = spawn_tileset_component.tileset.initializeTutorialLevelObstacleMap();


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
	tutorial_state = TutorialState::INTRO;
	player = createPlayer(renderer, { tilesize * 9, tilesize * 7 });
	createRightDoor(renderer, { tilesize * 18, tilesize * 5 });
	createArmorPlate(renderer, { tilesize * 14, tilesize * 5 });
	createPotion(renderer, { tilesize * 11, tilesize * 9 });
	createKey(renderer, { tilesize * 6, tilesize * 7 });
	registry.colors.insert(player, glm::vec3(1.f, 1.f, 1.f));
	renderer->player = player;
}
void WorldSystem::load_remote_location(int map_width, int map_height) {
	auto spawn_tileset_entity = Entity();
	TileSetComponent& spawn_tileset_component = registry.tilesets.emplace(spawn_tileset_entity);
	spawn_tileset_component.tileset.initializeTileTextureMap(7, 52);

	int tilesize = 64;

	std::vector<std::vector<int>> grass_map = spawn_tileset_component.tileset.initializeRemoteLocationMap();
	obstacle_map = spawn_tileset_component.tileset.initializeObstacleMap();


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
	float new_spawn_x = tilesize * 15;  // Adjust the spawn position if necessary
	float new_spawn_y = tilesize * 10;
	Motion& player_motion = registry.motions.get(player);  // Get player's motion component
	player_motion.position = { new_spawn_x, new_spawn_y };

	// the player position at the remote location
	//player = createPlayer(renderer, { tilesize * 15, tilesize * 15 });
	//createKey(renderer, { tilesize * 15, tilesize * 10 });
	//createKey(renderer, { tilesize * 7, tilesize * 10 });
	spaceship = createSpaceship(renderer, { tilesize * 7, tilesize * 10 });
	registry.colors.insert(spaceship, { 0.761f, 0.537f, 0.118f });
	//createPotion(renderer, { tilesize * 7, tilesize * 10 });
	//createPotion(renderer, { tilesize * 7, tilesize * 10 });
	//createArmorPlate(renderer, { tilesize * 7, tilesize * 10 });
	//createKey(renderer, { tilesize * 7, tilesize * 10 });
	renderer->player = player;
	createSpiderRobot(renderer, { tilesize * 13, tilesize * 10 });
	createSpiderRobot(renderer, { tilesize * 10, tilesize * 6 });
	createSpiderRobot(renderer, { tilesize * 9, tilesize * 6 });
	createSpiderRobot(renderer, { tilesize * 12, tilesize * 7 });
	createSpiderRobot(renderer, { tilesize * 12, tilesize * 9 });

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

		if (registry.spiderRobots.has(entity) && registry.players.has(entity_other)) {
			SpiderRobot& spider = registry.spiderRobots.get(entity);
			Player& player = registry.players.get(entity_other);

			// Check if the spider can attack
			if (spider.attack_timer <= 0.0f) {
				float attack_damage = 3.0f;
				player.current_health -= attack_damage;

				spider.attack_timer = spider.attack_cooldown;

				if (player.current_health <= 0) {
					if (!registry.deathTimers.has(entity_other)) {
						registry.deathTimers.emplace(entity_other);
						PlayerAnimation& pa = registry.animations.get(entity_other);
						pa.setState(AnimationState::DEAD, pa.current_dir);
						Mix_PlayChannel(-1, player_dead_sound, 0);
					}
				}
			}
		}

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
				}
				else if (robot.companion) {
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
				}
				else if (robot.companion) {
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
					}
					else pickup_item_name = "CompanionRobot";
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
				if (current_level == 0 && door.is_locked) {
					if (playerInventory->containsItem("Key")) {
						registry.notifications.clear();
						while (!notificationQueue.empty()) {
							notificationQueue.pop();
						}
						createNotification("Hint: Press [Q] to use an item.", 3.0f);
					}
					else {
						registry.notifications.clear();
						createNotification("You need a keycard to open this.", 3.0f);
					}
				}
				else {
					if (playerInventory->containsItem("Key")) {
						registry.notifications.clear();
						while (!notificationQueue.empty()) {
							notificationQueue.pop();
						}
						createNotification("Hint: Press [Q] to use an item.", 3.0f);
					}
					else {
						registry.notifications.clear();
						if (current_level != 0) {
							createNotification("Locked. I need a keycard. Maybe one of these robots would have it.", 3.0f);

						}
					}
				}

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
				}
			}
			
			Player& player = registry.players.get(entity);
			PlayerAnimation& pa = registry.animations.get(entity);
			if (player.current_health <= 0) {
				if (!registry.deathTimers.has(entity)) {
					registry.deathTimers.emplace(entity);
					pa.setState(AnimationState::DEAD, pa.current_dir);
					Mix_PlayChannel(-1, player_dead_sound, 0);
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
	static bool h_pressed = false; 

	if (renderer->helpOverlay.isVisible()) {
		if (key == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
			std::cout << "Help screen is active; ignoring clicks outside." << std::endl;
		}
		return; 
	} 

	if (show_start_screen) {
		if (renderer->show_start_screen && key == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
			switch (renderer->hovered_menu_index) {
			case 0: {
				show_start_screen = false;
				renderer->show_start_screen = false;
				std::vector<TEXTURE_ASSET_ID> cutscene_images = {
				TEXTURE_ASSET_ID::C1, TEXTURE_ASSET_ID::C2, TEXTURE_ASSET_ID::C3, TEXTURE_ASSET_ID::C4,
				TEXTURE_ASSET_ID::C5, TEXTURE_ASSET_ID::C6, TEXTURE_ASSET_ID::C7, TEXTURE_ASSET_ID::C8,
				TEXTURE_ASSET_ID::C9, TEXTURE_ASSET_ID::C10, TEXTURE_ASSET_ID::C11, TEXTURE_ASSET_ID::C12,
				TEXTURE_ASSET_ID::C13, TEXTURE_ASSET_ID::C14, TEXTURE_ASSET_ID::C15, TEXTURE_ASSET_ID::C16,
				TEXTURE_ASSET_ID::C17, TEXTURE_ASSET_ID::C18, TEXTURE_ASSET_ID::C19, TEXTURE_ASSET_ID::C20,
				TEXTURE_ASSET_ID::C21, TEXTURE_ASSET_ID::C22, TEXTURE_ASSET_ID::C23, TEXTURE_ASSET_ID::C24,
				TEXTURE_ASSET_ID::C25, TEXTURE_ASSET_ID::C26, TEXTURE_ASSET_ID::C27, TEXTURE_ASSET_ID::C28,
				TEXTURE_ASSET_ID::C29, TEXTURE_ASSET_ID::C30, TEXTURE_ASSET_ID::C31, TEXTURE_ASSET_ID::C32,
				TEXTURE_ASSET_ID::C33, TEXTURE_ASSET_ID::C34, TEXTURE_ASSET_ID::C35, TEXTURE_ASSET_ID::C36,
				TEXTURE_ASSET_ID::C37, TEXTURE_ASSET_ID::C38, TEXTURE_ASSET_ID::C39, TEXTURE_ASSET_ID::C40,
				TEXTURE_ASSET_ID::C41, TEXTURE_ASSET_ID::C42, TEXTURE_ASSET_ID::C43, TEXTURE_ASSET_ID::C44,
				TEXTURE_ASSET_ID::C45, TEXTURE_ASSET_ID::C46, TEXTURE_ASSET_ID::C47, TEXTURE_ASSET_ID::C48,
				TEXTURE_ASSET_ID::C49, TEXTURE_ASSET_ID::C50, TEXTURE_ASSET_ID::C51, TEXTURE_ASSET_ID::C52,
				TEXTURE_ASSET_ID::C53, TEXTURE_ASSET_ID::C54, TEXTURE_ASSET_ID::C55, TEXTURE_ASSET_ID::C56,
				TEXTURE_ASSET_ID::C57, TEXTURE_ASSET_ID::C58, TEXTURE_ASSET_ID::C59, TEXTURE_ASSET_ID::C60
				};

				triggerCutscene(cutscene_images);
				restart_game();
				break;
			}
			case 1: { // Load Previous Save
				show_start_screen = false;
				renderer->show_start_screen = false;
				renderer->tutorial_state = tutorial_state;
				std::cout << "Current tutorial state: " << static_cast<int>(tutorial_state) << std::endl;
				std::cout << "Current tutorial state: " << static_cast<int>(renderer->tutorial_state) << std::endl;
				break;
			}
			case 2: { // Help Screen
				renderer->toggleHelp();
				h_pressed = true;
				break;
			}
			case 3: { // Quit Game
				glfwSetWindowShouldClose(window, GL_TRUE);
				break;
			}
			default:
				break;
			}
			return;
		}
		return;
	}


	if (renderer->playing_cutscene) {
		if (key == GLFW_KEY_ENTER) {
			renderer->skipCutscene();
			
		}
		return;
	}

	
	if (tutorial_state != TutorialState::COMPLETED && action == GLFW_PRESS && key == GLFW_KEY_ENTER) {
		tutorial_state = TutorialState::COMPLETED;
		renderer->tutorial_state = tutorial_state;
		while (!notificationQueue.empty()) {
			notificationQueue.pop();
		}
		registry.notifications.entities.clear();
	}
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


	if (renderer->show_capture_ui) {
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
				uiScreenShown = true;
				if (tutorial_state == TutorialState::ROBOT_PARTS_HINT) {
					inventoryOpened = true;
				}
			}
			else {
				printf("Inventory closed.\n");
				auto& armorSlot = inventory.getArmorSlot();
				auto& weaponSlot = inventory.getWeaponSlot();

				if (!armorSlot.item.name.empty()) {
					bool addedToInventory = false;
					for (auto& slot : inventory.slots) {
						if (slot.item.name.empty() && slot.type != InventorySlotType::ARMOR && slot.type != InventorySlotType::WEAPON) {
							slot.item = armorSlot.item; // Move item to the first empty slot
							addedToInventory = true;
							break;
						}
					}

					if (!addedToInventory) {
						printf("No available slot to move armor item '%s'.\n", armorSlot.item.name.c_str());
						// Optionally handle this scenario (e.g., drop item, notify player, etc.)
					}

					armorSlot.item = {}; // Clear armor slot
				}

				if (!weaponSlot.item.name.empty()) {
					bool addedToInventory = false;
					for (auto& slot : inventory.slots) {
						if (slot.item.name.empty() && slot.type != InventorySlotType::ARMOR && slot.type != InventorySlotType::WEAPON) {
							slot.item = weaponSlot.item; // Move item to the first empty slot
							addedToInventory = true;
							break;
						}
					}

					if (!addedToInventory) {
						printf("No available slot to move weapon item '%s'.\n", weaponSlot.item.name.c_str());
						// Optionally handle this scenario (e.g., drop item, notify player, etc.)
					}

					weaponSlot.item = {}; // Clear weapon slot
				}
				uiScreenShown = false;
				if (tutorial_state == TutorialState::ROBOT_PARTS_HINT) {
					inventoryClosed = true;
			
				}
			}
			break;

		case GLFW_KEY_E:
			if (pickup_allowed && pickup_entity != Entity{}) {
				Inventory& inventory = registry.players.get(player).inventory;

				if (inventory.isFull()) {
					notificationQueue.emplace("Inventory is full! Cannot pick up " + pickup_item_name, 3.0f);
				//	Mix_PlayChannel(-1, error_sound, 0);
					break;
				}

				Mix_PlayChannel(-1, key_sound, 0);

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
					key_collected = true;
				}

				registry.remove_all_components_of(pickup_entity);

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

		if (current_level >= 5) {
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
				Mix_PlayChannel(-1, insert_card, 0);
				playerInventory->removeItem(selectedItem.name, 1);
				//	printf("removing key");
				if (playerInventory->slots[slot].item.name.empty() && slot < playerInventory->slots.size() - 1) {
					playerInventory->setSelectedSlot(slot);
				}
				return;
			}
		}

		if (current_level != 0) {
		notificationQueue.emplace("Get closer to the door!", 3.0f);
		}
		printf("No door in range to use key on.\n");
		return;
	}

	else if (selectedItem.name == "CompanionRobot") {

		if (current_level != 4) {
			vec2 placementPosition = getPlayerPlacementPosition();
			createCompanionRobot(renderer, placementPosition, selectedItem);

			playerInventory->removeItem(selectedItem.name, 1);

			if (playerInventory->slots[slot].item.name.empty() && slot < playerInventory->slots.size() - 1) {
				playerInventory->setSelectedSlot(slot);
			}
		}
		else {
			notificationQueue.emplace("Uh, too high pressure - can't place companion!", 3.0f);
		}
	}
	else if (selectedItem.name == "ArmorPlate") {
		Entity player_e = registry.players.entities[0];
		Player& player = registry.players.get(player_e);
		player.armor_stat += 15.0f;
		Mix_PlayChannel(-1, using_item, 0);
		playerInventory->removeItem(selectedItem.name, 1);

		if (playerInventory->slots[slot].item.name.empty() && slot < playerInventory->slots.size() - 1) {
			playerInventory->setSelectedSlot(slot);
		}
	}
	else if (selectedItem.name == "IceRobot") {

		if (current_level != 4) {
			vec2 placementPosition = getPlayerPlacementPosition();
			createCompanionIceRobot(renderer, placementPosition, selectedItem);

			playerInventory->removeItem(selectedItem.name, 1);

			if (playerInventory->slots[slot].item.name.empty() && slot < playerInventory->slots.size() - 1) {
				playerInventory->setSelectedSlot(slot);
			}
		}
		else {
			notificationQueue.emplace("Uh, too high pressure - can't place companion!", 3.0f);
		}
	}
	else if (selectedItem.name == "HealthPotion") {
		Entity player_e = registry.players.entities[0];
		Player& player = registry.players.get(player_e);
		if (player.current_health < player.max_health) {
			player.current_health += 30.f;
			Mix_PlayChannel(-1, using_item, 0);
			if (player.current_health > player.max_health) {
				player.current_health = player.max_health;
			}
			playerInventory->removeItem(selectedItem.name, 1);

			if (playerInventory->slots[slot].item.name.empty() && slot < playerInventory->slots.size() - 1) {
				playerInventory->setSelectedSlot(slot);
			}
		}
		else {
			std::queue<std::pair<std::string, float>> tempQueue;
			tempQueue.emplace("You cannot use the health potion at full health.", 3.0f);

			while (!notificationQueue.empty()) {
				tempQueue.emplace(notificationQueue.front());
				notificationQueue.pop();
			}
			std::swap(notificationQueue, tempQueue);
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
			std::queue<std::pair<std::string, float>> tempQueue;
			tempQueue.emplace("Can't use that here.", 3.0f);

			while (!notificationQueue.empty()) {
				tempQueue.emplace(notificationQueue.front());
				notificationQueue.pop();
			}
			std::swap(notificationQueue, tempQueue);
			return;
		}

		Entity player_e = registry.players.entities[0];
		Player& player = registry.players.get(player_e);

		if (player.dashCooldown > 0.f) {
			std::queue<std::pair<std::string, float>> tempQueue;
			tempQueue.emplace("Teleporter is on cooldown! Please wait before using it again.", 3.0f);

			while (!notificationQueue.empty()) {
				tempQueue.emplace(notificationQueue.front());
				notificationQueue.pop();
			}
			std::swap(notificationQueue, tempQueue);
			return;
		}

		// Calculate the teleport destination
		vec2 teleportDirection = normalize(registry.motions.get(player_e).target_velocity);
		if (glm::length(teleportDirection) == 0) {
			teleportDirection = vec2(1.f, 0.f); // Default direction
		}

		vec2 teleportDestination = player_motion.position + teleportDirection * 192.0f; // Example teleport distance

		// Check if the teleport destination is walkable directly in the code
		int tile_x = static_cast<int>(teleportDestination.x / 64.0f);
		int tile_y = static_cast<int>(teleportDestination.y / 64.0f);

		// Validate the teleportation destination
		if (tile_y < 0 || tile_y >= obstacle_map.size() ||
			tile_x < 0 || tile_x >= obstacle_map[0].size() ||
			obstacle_map[tile_y][tile_x] != 0) {
			std::queue<std::pair<std::string, float>> tempQueue;
			tempQueue.emplace("Can't use that here.", 3.0f);

			while (!notificationQueue.empty()) {
				tempQueue.emplace(notificationQueue.front());
				notificationQueue.pop();
			}
			std::swap(notificationQueue, tempQueue);
			return;
		}

		// Perform the teleport
		Mix_PlayChannel(-1, teleport_sound, 0);
		player_motion.position = teleportDestination;

		// Update player state
		player.isDashing = true;
		player.dashTimer = 0.7f;
		player.dashCooldown = 2.0f;
		player.lastDashDirection = teleportDirection;

		playerInventory->removeItem(selectedItem.name, 1);
		if (playerInventory->slots[slot].item.name.empty() && slot < playerInventory->slots.size() - 1) {
			playerInventory->setSelectedSlot(slot);
		}
	}



	else if (selectedItem.name == "Energy Core") {
		Entity player_e = registry.players.entities[0];
		Player& player = registry.players.get(player_e);
		player.max_stamina += 5.f;
		Mix_PlayChannel(-1, using_item, 0);
		player.current_stamina = std::min(player.current_stamina + 20.f, player.max_stamina);
		std::queue<std::pair<std::string, float>> tempQueue;
		tempQueue.emplace("Stamina increased!", 3.0f);

		while (!notificationQueue.empty()) {
			tempQueue.emplace(notificationQueue.front());
			notificationQueue.pop();
		}
		std::swap(notificationQueue, tempQueue);
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

	if (registry.iceRobotAnimations.has(renderer->currentRobotEntity)) {
		printf("Hello, World!\n");
		playerInventory->addCompanionRobot("IceRobot", robot.current_health, robot.attack, robot.speed);
		renderer->show_capture_ui = false;
		robot.showCaptureUI = false;
		uiScreenShown = false;
		registry.remove_all_components_of(renderer->currentRobotEntity);
	}
	else if (registry.robots.has(renderer->currentRobotEntity)) {
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
		if (playerInventory->isFull()) {
			printf("Inventory is full! Cannot add %s.\n", item.name.c_str());
			notificationQueue.emplace("Inventory is full! Cannot add " + item.name, 3.0f);
			continue; // Skip this item and move to the next
		}

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


			Mix_PlayChannel(-1, Upgrade, 0);
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
	Entity radiation_entity = *registry.radiations.entities.begin();
	Radiation& radiation = registry.radiations.get(radiation_entity);
	switch (level) {

	case 0:
		//registry.maps.clear();
		map_width = 20;
		map_height = 12;
		printf("loading remote level");
		screen.is_nighttime = false;
		radiation = { 0.0f, 0.0f };

		load_tutorial_level(20, 12);
		break;
	case 1:
		//registry.maps.clear();
		map_width = 21;
		map_height = 18;
		registry.maps.clear();
		printf("loading remote level");
		screen.is_nighttime = true;
		radiation = { 0.1f, 2.0f };

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
		radiation = { 0.3f, 3.0f };
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
		radiation = { 0.6f, 4.0f };
		load_second_level(40, 28);
		//generate_json(registry);
		break;
	case 4:
		// Setup for level 4
		registry.maps.clear();
		map_width = 20;
		map_height = 12;
		screen.is_nighttime = false;
		renderer->show_capture_ui = false;
		radiation = { 0.6f, 4.0f };
		load_third_level(20, 12);
		//generate_json(registry);
		break;

	case 5:
		// Setup for final level
		registry.maps.clear();
		map_width = 64;
		map_height = 40;
		screen.is_nighttime = true;
		renderer->show_capture_ui = false;
		radiation = { 1.0f, 5.0f };
		load_boss_level(64, 40);
		//generate_json(registry);
		break;
	default:
		radiation = { 0.0f, 0.0f };
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

// debug
void WorldSystem::triggerCutscene(const std::vector<TEXTURE_ASSET_ID>& images) {
	renderer->startCutscene(images);
}


void to_json(json& j, const WorldSystem& ws) {

	std::queue<std::pair<std::string, float>> temp = ws.notificationQueue;
	std::vector<std::pair<std::string, float>> vec;
	while (!temp.empty()) {
		vec.push_back(temp.front());
		temp.pop();
	}
	j = json{
		{"current_level", ws.get_current_level()},
		{"player", ws.get_player()},
		{"spaceship", ws.get_spaceship()},
		{"notificationQueue",vec},
		{"tutorial_state",ws.tutorial_state},

		{"introNotificationsAdded", ws.introNotificationsAdded},
		{"armorPickedUp", ws.armorPickedUp},
		{"potionPickedUp", ws.potionPickedUp},
		{"movementHintShown",ws.movementHintShown},
		{"pickupHintShown",ws.pickupHintShown},
		{"sprintHintShown",ws.sprintHintShown},
		{"robotPartsCount", ws.robotPartsCount},
		{"inventoryOpened", ws.inventoryOpened},
		{"inventoryClosed", ws.inventoryClosed},
		{"inventoryHintShown",ws.inventoryHintShown},
		{"attackNotificationsAdded",ws.attackNotificationsAdded},
		{"keyPickedUp",ws.keyPickedUp},
	};
}
void from_json(const json& j, WorldSystem& ws) {
	ws.set_current_level(j["current_level"]);
	ws.set_player(j["player"]);
	ws.set_spaceship(j["spaceship"]);
	j.at("tutorial_state").get_to(ws.tutorial_state);
	j.at("introNotificationsAdded").get_to(ws.introNotificationsAdded);
	j.at("armorPickedUp").get_to(ws.armorPickedUp);
	j.at("potionPickedUp").get_to(ws.potionPickedUp);
	j.at("movementHintShown").get_to(ws.movementHintShown);
	j.at("pickupHintShown").get_to(ws.pickupHintShown);
	j.at("sprintHintShown").get_to(ws.sprintHintShown);
	j.at("robotPartsCount").get_to(ws.robotPartsCount);
	j.at("inventoryOpened").get_to(ws.inventoryOpened);
	j.at("inventoryClosed").get_to(ws.inventoryClosed);
	j.at("inventoryHintShown").get_to(ws.inventoryHintShown);
	j.at("attackNotificationsAdded").get_to(ws.attackNotificationsAdded);
	j.at("keyPickedUp").get_to(ws.keyPickedUp);

	std::vector<std::pair<std::string, float>> vec = j["notificationQueue"].get<std::vector<std::pair<std::string, float>>>();
	std::queue<std::pair<std::string, float>> temp;
	for (const auto& item : vec) {
		temp.push(item);
	}
	ws.notificationQueue = temp;
}
