// internal
#include "physics_system.hpp"
#include "world_init.hpp"
#include "math_utils.hpp"
#include "render_system.hpp"
#include <queue>


void bound_check(Motion& mo);

void dumb_ai(Motion& mo);

std::vector<std::pair<int, int>> bfs(const std::vector<std::vector<int>>& tile_map,
	std::pair<int, int> start, std::pair<int, int> end);

std::pair<int, int> translate_vec2(Motion x);

vec2 translate_pair(std::pair<int, int> p);

Direction bfs_ai(Motion& mo);

float lerp_float(float start, float end, float t);

void lerp_rotate(Motion& motion);

vec2 lerp_move(vec2 start, vec2 end, float t);

void attackbox_check(Entity en);

bool attack_hit(const Motion& motion1, const attackBox& motion2);

bool shouldmv(Entity e);

bool shouldmv(Entity e);

bool shouldattack(Entity e);

bool shouldidle(Entity e);

bool bossShouldmv(Entity e);

bool bossShouldattack(Entity e);

bool bossShouldidle(Entity e);

void handelRobot(Entity entity, float elapsed_ms);
void handelBossRobot(Entity entity, float elapsed_ms);

// Returns the local bounding coordinates scaled by the current size of the entity
vec2 get_bounding_box(const Motion& motion)
{
	// abs is to avoid negative scale due to the facing direction.
	return { abs(motion.bb.x), abs(motion.bb.y) };
}


// Function to check if a point is inside a triangle (for mesh collision detection) - not used
bool point_in_triangle(vec2 pt, Triangle tri) {
	vec2 v0 = tri.v3 - tri.v1;
	vec2 v1 = tri.v2 - tri.v1;
	vec2 v2 = pt - tri.v1;

	float dot00 = dot(v0, v0);
	float dot01 = dot(v0, v1);
	float dot02 = dot(v0, v2);
	float dot11 = dot(v1, v1);
	float dot12 = dot(v1, v2);

	float invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
	float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

	return (u >= 0) && (v >= 0) && (u + v < 1);
}

bool PhysicsSystem::checkMeshCollision(const Motion& motion1, const Motion& motion2, const Mesh* mesh) {
	if (!mesh) return false;


	vec2 box_half_size = get_bounding_box(motion2) / 2.f;
	vec2 box_pos = motion2.position;

	vec2 box_min = box_pos - box_half_size;
	vec2 box_max = box_pos + box_half_size;

	for (const auto& vertex : mesh->vertices) {
		vec2 transformed_pos = motion1.position + vec2(vertex.position.x * motion1.scale.x,
			vertex.position.y * motion1.scale.y);

		if (transformed_pos.x >= box_min.x && transformed_pos.x <= box_max.x &&
			transformed_pos.y >= box_min.y && transformed_pos.y <= box_max.y) {
			return true;
		}
	}
	return false;
}

std::vector<Triangle> spaceship_mesh = {
	{{0.0f, 0.5f}, {-0.15f, 0.2f}, {0.15f, 0.2f}},  // Face 1 (f 1//1 2//2 3//3)
	{{-0.15f, 0.2f}, {-0.15f, -0.4f}, {0.15f, -0.4f}}, // Face 2 (f 2//2 4//4 5//5)
	{{-0.15f, 0.2f}, {0.15f, -0.4f}, {0.15f, 0.2f}},  // Face 3 (f 2//2 5//5 3//3)
	{{-0.15f, -0.4f}, {0.15f, -0.4f}, {0.0f, -0.6f}}, // Face 4 (f 4//4 5//5 6//6)
	{{0.0f, -0.6f}, {0.05f, -0.5f}, {-0.05f, -0.5f}}, // Face 5 (f 6//6 7//7 8//8)
	{{-0.15f, -0.4f}, {0.0f, -0.6f}, {0.05f, -0.5f}}, // Face 6 (f 4//4 6//6 7//7)
	{{0.0f, -0.6f}, {0.05f, -0.5f}, {-0.1f, -0.6f}},  // Face 7 (f 6//6 7//7 8//8)
	{{0.1f, -0.45f}, {0.15f, -0.45f}, {0.1f, -0.6f}}, // Face 8 (f 9//9 10//10 12//12)
	{{0.1f, -0.45f}, {0.15f, -0.45f}, {0.15f, -0.6f}}, // Face 9 (f 9//9 11//11 12//12)
	{{-0.15f, -0.45f}, {-0.1f, -0.45f}, {-0.15f, -0.6f}}, // Face 10 (f 13//13 14//14 16//16)
	{{-0.15f, -0.6f}, {-0.1f, -0.6f}, {-0.1f, -0.45f}}  // Face 11 (f 13//13 15//15 16//16)
};




// This is a SUPER APPROXIMATE check that puts a circle around the bounding boxes and sees
// if the center point of either object is inside the other's bounding-box-circle. You can
// surely implement a more accurate detection
bool collides(const Motion& motion1, const Motion& motion2)
{
	//float scale_factor = 0.35;

	vec2 size1 = get_bounding_box(motion1);
	vec2 size2 = get_bounding_box(motion2);

	vec2 pos1_min = motion1.position - (size1 / 2.f);
	vec2 pos1_max = motion1.position + (size1 / 2.f);

	vec2 pos2_min = motion2.position - (size2 / 2.f);
	vec2 pos2_max = motion2.position + (size2 / 2.f);

	bool overlap_x = (pos1_min.x <= pos2_max.x && pos1_max.x >= pos2_min.x);
	bool overlap_y = (pos1_min.y <= pos2_max.y && pos1_max.y >= pos2_min.y);

	return overlap_x && overlap_y;

}
struct close_enemy {
	Entity i;
	float dist;
};
close_enemy companion_close_enemy(Entity entity) {
	auto& robot_registry = registry.robots;
	float curr_min = std::numeric_limits<float>::max();
	Motion companion = registry.motions.get(entity);
	close_enemy temp;
	temp.i = entity;
	temp.dist = 0.f;
	for (uint i = 0; i < robot_registry.size(); i++) {
		Entity e = robot_registry.entities[i];
		Motion m = registry.motions.get(e);
		if (e.id != entity.id) {
			if (length(companion.position - m.position) < curr_min) {
				curr_min = length(companion.position - m.position);
				temp.i = e;
				temp.dist = curr_min;
			}
		}
	}
	return temp;
}
void handelCompanion(Entity entity, float elapsed_ms) {
	close_enemy temp = companion_close_enemy(entity);
	Motion& motion = registry.motions.get(entity);

	bool attacking = false;
	if (temp.dist < 384.f && temp.i.id != entity.id && registry.robotAnimations.get(entity).current_state != RobotState::DEAD) {
		RobotAnimation& ra = registry.robotAnimations.get(entity);
		motion.velocity = vec2(0);
		if (ra.current_state != RobotState::ATTACK) {
			ra.setState(RobotState::ATTACK, ra.current_dir);
		}
		else if (ra.current_frame == ra.getMaxFrames() - 1) {
			ra.current_frame = 0;
			Robot& ro = registry.robots.get(entity);
			//std::cout << "fire shot" << std::endl;
		
			Motion& enemy_motion = registry.motions.get(temp.i);
			vec2 target_velocity = normalize((enemy_motion.position - motion.position)) * 85.f;
			vec2 temp = motion.position - enemy_motion.position;
			float angle = atan2(temp.y, temp.x);
			angle += 3.14;
			createProjectile(motion.position, target_velocity, angle, ro.ice_proj, true, false);
		}
		attacking = true;
	}

	if (!attacking && registry.robotAnimations.get(entity).current_state != RobotState::DEAD) {
		Direction a = bfs_ai(motion);
		RobotAnimation& ra = registry.robotAnimations.get(entity);
		ra.setState(RobotState::WALK, a);
	}
	RobotAnimation& ra = registry.robotAnimations.get(entity);
	if (registry.robots.has(entity)) {
		Robot& ro = registry.robots.get(entity);
		if (ro.current_health <= 0) {
			if (!ro.should_die) {
				ro.should_die = true;
				ra.setState(RobotState::DEAD, ra.current_dir);
				ro.death_cd = ra.getMaxFrames() * ra.FRAME_TIME * 1000.f;
				if (ro.isCapturable) {
					ro.showCaptureUI = true;
					ro.current_health = ro.max_health / 2;
				}
				else {
					ro.death_cd -= elapsed_ms;

					if (ro.death_cd < 0) {
						registry.remove_all_components_of(entity);
					}

				}
			}
			else {
				ro.death_cd -= elapsed_ms;
				if (ro.death_cd < 0) {
					registry.remove_all_components_of(entity);
				}
			}
		}
	}
}

void handelRobot(Entity entity, float elapsed_ms) {
	Motion& motion = registry.motions.get(entity);
	RobotAnimation& ra = registry.robotAnimations.get(entity);

	Robot& ro = registry.robots.get(entity);
	if (ro.companion) {
		handelCompanion(entity, elapsed_ms);
		return;
	}

	if (shouldmv(entity) && registry.robotAnimations.get(entity).current_state != RobotState::DEAD) {
		Direction a = bfs_ai(motion);
		RobotAnimation& ra = registry.robotAnimations.get(entity);
		ra.setState(RobotState::WALK, a);
	}

	if (shouldattack(entity) && registry.robotAnimations.get(entity).current_state != RobotState::DEAD) {
		RobotAnimation& ra = registry.robotAnimations.get(entity);
		motion.velocity = vec2(0);
		if (ra.current_state != RobotState::ATTACK) {
			ra.setState(RobotState::ATTACK, ra.current_dir);
		}
		else if (ra.current_frame == ra.getMaxFrames() - 1) {
			ra.current_frame = 0;
			Robot& ro = registry.robots.get(entity);
			//std::cout << "fire shot" << std::endl;
			Entity player = registry.players.entities[0];
			Motion& player_motion = registry.motions.get(player);
			vec2 target_velocity = normalize((player_motion.position - motion.position)) * 85.f;
			vec2 temp = motion.position - player_motion.position;
			float angle = atan2(temp.y, temp.x);
			angle += 3.14;
			createProjectile(motion.position, target_velocity, angle, ro.ice_proj, false, false);
			ro.ice_proj = !ro.ice_proj;
		}
	}


	if (shouldidle(entity) && registry.robotAnimations.get(entity).current_state != RobotState::DEAD) {
		motion.velocity = vec2(0);
		ra.setState(RobotState::IDLE, ra.current_dir);
	}
	if (registry.robots.has(entity)) {
		Robot& ro = registry.robots.get(entity);
		if (ro.current_health <= 0) {
			if (!ro.should_die) {
				ro.should_die = true;
				ra.setState(RobotState::DEAD, ra.current_dir);
				ro.death_cd = ra.getMaxFrames() * ra.FRAME_TIME * 1000.f;
				if (ro.isCapturable) {
					ro.showCaptureUI = true;
					ro.current_health = ro.max_health / 2;
				}
				else {
					ro.death_cd -= elapsed_ms;

					if (ro.death_cd < 0) {
						registry.remove_all_components_of(entity);
					}
					
				}
			}
			else {
				ro.death_cd -= elapsed_ms;
				if (ro.death_cd < 0) {
					registry.remove_all_components_of(entity);
				}
			}
		}
	}
}

void handelBossRobot(Entity entity, float elapsed_ms) {
	Motion& motion = registry.motions.get(entity);
	BossRobotAnimation& ra = registry.bossRobotAnimations.get(entity);

	static float shoot_timer = 0.0f; // Timer for shooting bullets
	const float shoot_interval = 1.5f; // Time between shots
	const float health_loss_distance = 124.0f; // Proximity distance for health loss
	const float max_health_loss_time = 10.0f; // Time in seconds to start losing health
	const int health_loss_amount = 10; // Amount of health lost per second
	static float near_player_timer = 0.0f; // Timer for tracking time near player

	Entity player = registry.players.entities[0];
	Motion& player_motion = registry.motions.get(player);
	float distance_to_player = glm::distance(motion.position, player_motion.position);

	// Health loss logic if the player is within proximity
	if (distance_to_player < health_loss_distance) {
		near_player_timer += elapsed_ms / 1000.0f;
		if (near_player_timer >= max_health_loss_time) {
			// Lose health if the player has been near for too long
			Player& pl = registry.players.get(player);
			pl.current_health -= health_loss_amount;
			near_player_timer = 0.0f;
		}
	}
	else {
		// Reset the timer if the player is not near
		near_player_timer = 0.0f;
	}

	if (bossShouldmv(entity) && ra.current_state != BossRobotState::DEAD) {
		Direction a = bfs_ai(motion);
		ra.setState(BossRobotState::WALK, a);
	}

	// Check if the player is within attack range
	if (bossShouldattack(entity)) {
		motion.velocity = vec2(0);
			if (ra.current_state != BossRobotState::ATTACK) {
				ra.setState(BossRobotState::ATTACK, ra.current_dir);
				shoot_timer = 0.0f;
			}
			else {
				shoot_timer += elapsed_ms / 1000.0f;
				if (shoot_timer >= shoot_interval) {
					shoot_timer = 0.0f;

					vec2 target_position = player_motion.position;
					vec2 target_velocity = normalize(target_position - motion.position) * 85.0f;

					// Create the projectile
					createProjectile(motion.position, target_velocity, atan2(target_velocity.y, target_velocity.x), 10, false, true);
				}
			}
	}

	if (shouldidle(entity) && ra.current_state != BossRobotState::DEAD) {
		motion.velocity = vec2(0);
		ra.setState(BossRobotState::IDLE, ra.current_dir);
	}

	// Handle death logic
	if (registry.bossRobots.has(entity)) {
		BossRobot& ro = registry.bossRobots.get(entity);
		if (ro.current_health <= 0) {
			if (!ro.should_die) {
				ro.should_die = true;
				ra.setState(BossRobotState::DEAD, ra.current_dir);
				ro.death_cd = ra.getMaxFrames() * ra.FRAME_TIME * 1000.f;
			}
			else {
				ro.death_cd -= elapsed_ms;
				if (ro.death_cd < 0) {
					registry.remove_all_components_of(entity);
				}
			}
		}
	}
}


void PhysicsSystem::step(float elapsed_ms, WorldSystem* world)
{

	ComponentContainer<Motion>& motion_container = registry.motions;
	// Move entities based on the time passed, ensuring entities move at consistent speeds
	std::vector<Entity> should_remove;
	auto& motion_registry = registry.motions;
	for (uint i = 0; i < motion_registry.size(); i++)
	{
		Motion& motion = motion_registry.components[i];
		Entity entity = motion_registry.entities[i];
		float step_seconds = elapsed_ms / 1000.f;

		bool flag = true;
		if (registry.tiles.has(entity)) {
			continue;
		}

		lerp_rotate(motion);

		if (registry.robots.has(entity)) {
			handelRobot(entity, elapsed_ms);
		}

		if (registry.bossRobots.has(entity)) {
			handelBossRobot(entity,elapsed_ms);
		}
		vec2 pos = motion.position;

		if (registry.players.has(entity)) {
			Player& p = registry.players.get(entity);
			if (!p.slow) {
				motion.velocity.x = exp_inter(motion.target_velocity.x, motion.velocity.x, step_seconds * 100.f);
				motion.velocity.y = exp_inter(motion.target_velocity.y, motion.velocity.y, step_seconds * 100.f);
			}
			else {
				motion.velocity.x = exp_inter(motion.target_velocity.x * 0.75, motion.velocity.x, step_seconds * 100.f);
				motion.velocity.y = exp_inter(motion.target_velocity.y * 0 / 75, motion.velocity.y, step_seconds * 100.f);
				p.slow_count_down -= elapsed_ms;
				if (p.slow_count_down <= 0) {
					p.slow = false;
				}
			}
			if (p.isDashing) {
				// Determine dash direction based on player orientation
				auto& animation = registry.animations.get(entity);
				vec2 dashDirection = vec2(0.f, 0.f);

				switch (animation.current_dir) {
				case Direction::DOWN: dashDirection = vec2(0.f, 1.f); break;
				case Direction::UP: dashDirection = vec2(0.f, -1.f); break;
				case Direction::LEFT: dashDirection = vec2(-1.f, 0.f); break;
				case Direction::RIGHT: dashDirection = vec2(1.f, 0.f); break;
				default: dashDirection = vec2(0.f, 0.f);
				}

				// Normalize the dash direction to ensure consistent speed
				dashDirection = glm::normalize(dashDirection);

				// Initialize target position if not already set
				if (!p.dashTargetSet) {
					p.dashStartPosition = motion.position;
					p.dashTarget = motion.position + dashDirection * (64.0f * 5.0f); // Increased dash distance
					p.dashTargetSet = true;

					// Debugging output for dash target
					printf("Dash Target: (%.2f, %.2f)\n", p.dashTarget.x, p.dashTarget.y);
				}

				// Interpolate towards the target position
				float t = glm::min(1.0f, p.dashSpeed * (elapsed_ms / 1000.f));
				motion.position.x = exp_inter(p.dashTarget.x, motion.position.x, step_seconds * 10.f);
				motion.position.y = exp_inter(p.dashTarget.y, motion.position.y, step_seconds * 10.f);
				// Check if the dash duration has expired
				p.dashTimer -= elapsed_ms;
				if (p.dashTimer <= 0.f || glm::length(p.dashTarget - motion.position) < 1.0f) {
					p.isDashing = false;
					p.dashTargetSet = false;
					printf("Dash completed at: (%.2f, %.2f)\n", motion.position.x, motion.position.y);
				}
			}



			else if (p.dashCooldown > 0.f) {
				// Reduce cooldown when not dashing
				p.dashCooldown -= elapsed_ms / 1000.f;
				printf("Dash Cooldown Remaining: %.2f seconds\n", p.dashCooldown);
			}






		}
		else {
			motion.velocity.x = linear_inter(motion.target_velocity.x, motion.velocity.x, step_seconds * 100.f);
			motion.velocity.y = linear_inter(motion.target_velocity.y, motion.velocity.y, step_seconds * 100.f);
		}

		motion.position += motion.velocity * step_seconds;


		if (registry.projectile.has(entity)) {
			if (motion.position.x < 0.0f || motion.position.x > map_width * 64.f ||
				motion.position.y < 0.0f || motion.position.y > map_height * 64.f) {
				registry.remove_all_components_of(entity);
				printf("entity removed");
				continue;
			}
		}
		if (registry.robots.has(entity) || registry.players.has(entity)) {
			attackbox_check(entity);
		}
		if (registry.bossRobots.has(entity) || registry.players.has(entity)) {
			attackbox_check(entity);
		}
		if (!registry.tiles.has(entity) && !registry.robots.has(entity)) {

			// note starting j at i+1 to compare all (i,j) pairs only once (and to not compare with itself)
			for (uint j = 0; j < motion_container.components.size(); j++)
			{
				Motion& motion_j = motion_container.components[j];
				if (collides(motion, motion_j))
				{
					Entity entity_j = motion_container.entities[j];
					// Create a collisions event
					// We are abusing the ECS system a bit in that we potentially insert muliple collisions for the same entity

					if (registry.tiles.has(entity_j)) {
						if (!registry.tiles.get(entity_j).walkable) {
							motion.position = pos;
							motion.velocity = vec2(0);
							if (registry.players.has(entity)) {
								world->play_collision_sound();
							}
							if (registry.projectile.has(entity)) {
								flag = false;
								//should_remove.push_back(entity_j);
								should_remove.push_back(entity);
							}
						}
					}


				}
			}

			if (!registry.projectile.has(entity)) {
				bound_check(motion);
			}
		}
		if (!registry.spaceships.has(entity)) {
			Entity spaceship_entity;
			for (Entity e : registry.spaceships.entities) {
				spaceship_entity = e;
				break;
			}

			if (registry.meshPtrs.has(spaceship_entity)) {
				const Mesh* mesh = registry.meshPtrs.get(spaceship_entity);
				if (mesh && checkMeshCollision(registry.motions.get(spaceship_entity), motion, mesh)) {

					motion.position = pos;
					motion.velocity = vec2(0.f);
					motion.target_velocity = vec2(0.f);

					if (registry.players.has(entity)) {
						world->play_collision_sound();
					}
					if (registry.projectile.has(entity)) {
						registry.remove_all_components_of(entity);
					}

				}
				
			}
		}
	}

	for (Entity e : should_remove) {
		registry.remove_all_components_of(e);
	}


	// Add after motion.position += motion.velocity * step_seconds;
	// and before the robot/player checks

	// Check mesh collision with spaceship



	// Check for collisions between all moving entities
	for (uint i = 0; i < motion_container.components.size(); i++)
	{
		Motion& motion_i = motion_container.components[i];
		Entity entity_i = motion_container.entities[i];
		if (registry.tiles.has(entity_i)) {
			continue;
		}

		// note starting j at i+1 to compare all (i,j) pairs only once (and to not compare with itself)
		for (uint j = i + 1; j < motion_container.components.size(); j++)
		{
			Motion& motion_j = motion_container.components[j];
			Entity entity_j = motion_container.entities[j];
			if (registry.tiles.has(entity_j)) {
				continue;
			}
			if (collides(motion_i, motion_j))
			{
				// Create a collisions event
				// We are abusing the ECS system a bit in that we potentially insert muliple collisions for the same entity
				if (registry.doors.has(entity_j)) {
					Door& door = registry.doors.get(entity_j);

					// Block player if the door is locked or not fully open
					if (door.is_locked || !door.is_open) {
						motion_i.position = motion_i.position - motion_i.velocity * (elapsed_ms / 1000.f); 
						motion_i.velocity = vec2(0);
						printf("Player blocked by door.\n");
					}

					if (registry.projectile.has(entity_i)) {
						registry.remove_all_components_of(entity_i);
					}
				}

				registry.collisions.emplace_with_duplicates(entity_i, entity_j);
				registry.collisions.emplace_with_duplicates(entity_j, entity_i);
			}
		}
	}

	registry.attackbox.clear();
}

void dumb_ai(Motion& mo) {
	Entity player = registry.players.entities[0];
	Motion& player_motion = registry.motions.get(player);
	mo.target_velocity = glm::normalize((player_motion.position - mo.position)) * vec2(50);
}

void bound_check(Motion& mo) {
	Entity T = registry.maps.entities[0];
	T_map m = registry.maps.get(T);

	// Calculate the boundary based on the map size and tile size
	float map_width_px = m.tile_size * m.tile_map[0].size();
	float map_height_px = m.tile_size * m.tile_map.size();
	//printf("new_grass_map size: %d x %d\n", m.tile_map[0].size(), m.tile_map.size());
	// Check the boundaries and adjust position if out of bounds
	mo.position.x = max(min(map_width_px - (mo.scale.x / 2), mo.position.x), mo.scale.x / 2);
	mo.position.y = max(min(map_height_px - (mo.scale.y / 2), mo.position.y), mo.scale.y / 2);
}

Direction bfs_ai(Motion& mo) {
	Entity player = registry.players.entities[0];
	Motion& player_motion = registry.motions.get(player);

	Entity T = registry.maps.entities[0];
	T_map m = registry.maps.get(T);

	std::pair<int, int> end = translate_vec2(player_motion);
	std::pair<int, int> start = translate_vec2(mo);

	std::vector<std::pair<int, int>> temp = bfs(m.tile_map, start, end);
	if (!temp.empty()) {
		vec2 bk = translate_pair(temp.back());
		//std::cout << "BK first: " << bk.x << " BK second: " << bk.y << std::endl;
		//std::cout << "Player first: " << player_motion.position.x << " Player second: " << player_motion.position.y << std::endl;


		if (temp.size() >= 2) {
			vec2 target = translate_pair(temp[1]);
			mo.velocity = normalize(vec2(target.x + 32, target.y + 32) - mo.position) * 64.f;
			if (start.first > temp[1].first) {
				return Direction::UP;
			}
			if (start.first < temp[1].first) {
				return Direction::DOWN;
			}
			if (start.second < temp[1].second) {
				return Direction::RIGHT;
			}
			if (start.second > temp[1].second) {
				return Direction::LEFT;
			}
		}
		else {
			vec2 target = translate_pair(temp[0]);
			mo.velocity = normalize(vec2(target.x + 32, target.y + 32) - mo.position) * 64.f;
		}

	}
	return Direction::LEFT;
}



std::pair<int, int> translate_vec2(Motion x) {
	Entity T = registry.maps.entities[0];
	T_map m = registry.maps.get(T);


	std::pair<int, int> temp;
	temp.first = (int)(x.position.y / m.tile_size);
	temp.second = (int)(x.position.x / m.tile_size);

	//std::cout << "Start first: " << temp.first << " Start second: " << temp.second << std::endl;

	return temp;
}

vec2 translate_pair(std::pair<int, int> p) {
	Entity T = registry.maps.entities[0];
	T_map m = registry.maps.get(T);
	vec2 temp;
	temp.x = p.second * m.tile_size;
	temp.y = p.first * m.tile_size;
	return temp;
}


std::vector<std::pair<int, int>> bfs(const std::vector<std::vector<int>>& tile_map, std::pair<int, int> start, std::pair<int, int> end) {
	int rows = tile_map.size();
	int cols = tile_map[0].size();

	/*if (start.first < 0 || start.first >= rows || start.second < 0 || start.second >= cols ||
		end.first < 0 || end.first >= rows || end.second < 0 || end.second >= cols ||
		tile_map[start.first][start.second] != 0 || tile_map[end.first][end.second] != 0) {
		return {};
	}*/

	int dirX[4] = { 0, 0, -1, 1 };
	int dirY[4] = { -1, 1, 0, 0 };

	std::vector<std::vector<bool>> visited(rows, std::vector<bool>(cols, false));

	std::queue<std::vector<std::pair<int, int>>> path_q;
	std::vector<std::pair<int, int>> temp;
	temp.push_back(start);

	path_q.push(temp);
	visited[start.first][start.second] = true;


	while (!path_q.empty()) {
		std::vector<std::pair<int, int>> current_path = path_q.front();
		path_q.pop();

		std::pair<int, int> current = current_path.back();

		if (current == end) {
			return current_path;
		}


		for (int i = 0; i < 4; ++i) {
			int x = current.first + dirX[i];
			int y = current.second + dirY[i];

			if (x >= 0 && x < rows && y >= 0 && y < cols &&
				tile_map[x][y] == 0 && !visited[x][y]) {
				visited[x][y] = true;
				std::vector<std::pair<int, int>> temp1 = current_path;
				std::pair<int, int> p;
				p.first = x;
				p.second = y;
				temp1.push_back(p);
				path_q.push(temp1);
			}
		}
	}

	return {};
}

float lerp_float(float start, float end, float t) {
	return start * (1.f - t) + end * t;
}

vec2 lerp_move(vec2 start, vec2 end, float t) {
	return start * vec2(1.f - t) + end * vec2(t);
}

void lerp_rotate(Motion& motion) {
	if (motion.t > 1.0f) {
		motion.t = 0;
		motion.should_rotate = false;
	}
	if (motion.should_rotate) {
		motion.angle = lerp_float(motion.start_angle, motion.end_engle, motion.t);
		motion.t += 0.01;
	}
}

void attackbox_check(Entity en) {
	ComponentContainer<attackBox>& attack_container = registry.attackbox;

	for (int i = 0; i < attack_container.size(); i++) {
		attackBox& attack_i = attack_container.components[i];
		Entity entity_i = attack_container.entities[i];

		Motion mo = registry.motions.get(en);

		if (attack_hit(mo, attack_i)) {
			if (registry.robots.has(en) && attack_i.friendly) {
				Robot& ro = registry.robots.get(en);
				if (!ro.companion) {
					ro.current_health -= attack_i.dmg;
				}
			}
			if (registry.bossRobots.has(en) && attack_i.friendly) {
				BossRobot& ro = registry.bossRobots.get(en);
				ro.current_health -= attack_i.dmg;
			}
			if (registry.players.has(en) && !attack_i.friendly) {
				Player& pl = registry.players.get(en);

				if (pl.armor_stat > 0) {
					float remaining_damage = attack_i.dmg - pl.armor_stat;
					pl.armor_stat -= attack_i.dmg;
					if (pl.armor_stat < 0) {
						pl.armor_stat = 0;
					}
					if (remaining_damage > 0) {
						pl.current_health -= remaining_damage;
					}
				}
				else {
					pl.current_health -= attack_i.dmg;
				}
			}
		}
	}
}


bool attack_hit(const Motion& motion1, const attackBox& motion2)
{

	vec2 size1 = get_bounding_box(motion1);
	vec2 size2 = { abs(motion2.bb.x), abs(motion2.bb.y) };;

	vec2 pos1_min = motion1.position - (size1 / 2.f);
	vec2 pos1_max = motion1.position + (size1 / 2.f);

	vec2 pos2_min = motion2.position - (size2 / 2.f);
	vec2 pos2_max = motion2.position + (size2 / 2.f);

	bool overlap_x = (pos1_min.x <= pos2_max.x && pos1_max.x >= pos2_min.x);
	bool overlap_y = (pos1_min.y <= pos2_max.y && pos1_max.y >= pos2_min.y);

	return overlap_x && overlap_y;

}

bool inbox(const Motion& motion1, vec2 box, vec2 pos)
{

	vec2 size1 = get_bounding_box(motion1);
	vec2 size2 = { abs(box.x), abs(box.y) };;

	vec2 pos1_min = motion1.position - (size1 / 2.f);
	vec2 pos1_max = motion1.position + (size1 / 2.f);

	vec2 pos2_min = pos - (size2 / 2.f);
	vec2 pos2_max = pos + (size2 / 2.f);

	bool overlap_x = (pos1_min.x <= pos2_max.x && pos1_max.x >= pos2_min.x);
	bool overlap_y = (pos1_min.y <= pos2_max.y && pos1_max.y >= pos2_min.y);

	return overlap_x && overlap_y;

}


bool shouldmv(Entity e) {
	Robot r = registry.robots.get(e);
	Motion m = registry.motions.get(e);

	Entity pl = registry.players.entities[0];
	Motion plm = registry.motions.get(pl);
	return inbox(plm, r.search_box, m.position) && !inbox(plm, r.attack_box, m.position) && !inbox(plm, r.panic_box, m.position);
}

bool shouldattack(Entity e) {
	Robot r = registry.robots.get(e);
	Motion m = registry.motions.get(e);

	Entity pl = registry.players.entities[0];
	Motion plm = registry.motions.get(pl);
	return inbox(plm, r.attack_box, m.position) && !inbox(plm, r.panic_box, m.position);
}

bool shouldidle(Entity e) {
	Robot r = registry.robots.get(e);
	Motion m = registry.motions.get(e);

	Entity pl = registry.players.entities[0];
	Motion plm = registry.motions.get(pl);
	return inbox(plm, r.panic_box, m.position);
}

bool bossShouldmv(Entity e) {
	BossRobot r = registry.bossRobots.get(e);
	Motion m = registry.motions.get(e);

	Entity pl = registry.players.entities[0];
	Motion plm = registry.motions.get(pl);
	return inbox(plm,r.search_box,m.position)&&!inbox(plm, r.attack_box, m.position)&&!inbox(plm, r.panic_box, m.position);
}

bool bossShouldattack(Entity e) {
    BossRobot r = registry.bossRobots.get(e);
    Motion m = registry.motions.get(e);

    Entity pl = registry.players.entities[0];
    Motion plm = registry.motions.get(pl);
    
    float attack_range = 400; // Adjust this value for the attack range
    return glm::distance(plm.position, m.position) <= attack_range; // Check if player is within attack range
}

bool bossShouldidle(Entity e) {
	BossRobot r = registry.bossRobots.get(e);
	Motion m = registry.motions.get(e);

	Entity pl = registry.players.entities[0];
	Motion plm = registry.motions.get(pl);
	return inbox(plm, r.panic_box, m.position);
}