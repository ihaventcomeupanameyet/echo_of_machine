// internal
#include "physics_system.hpp"
#include "world_init.hpp"
#include "math_utils.hpp"
#include "render_system.hpp"
#include <queue>

#include <vector>
#include <map>
#include <cmath>
#include <algorithm>


void bound_check(Motion& mo);

void dumb_ai(Motion& mo);

std::vector<std::pair<int, int>> bfs(const std::vector<std::vector<int>>& tile_map,
	std::pair<int, int> start, std::pair<int, int> end);

std::vector<std::pair<int, int>> a_star(const std::vector<std::vector<int>>& tile_map, 
        std::pair<int, int> start, std::pair<int, int> end);

std::pair<int, int> translate_vec2(Motion x);

vec2 translate_pair(std::pair<int, int> p);

Direction bfs_ai(Motion& mo);

Direction a_star_ai(Motion& mo);

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

bool spiderShouldmv(Entity e);

bool spiderShouldattack(Entity e);

bool bossShouldidle(Entity e);

void handelRobot(Entity entity, float elapsed_ms);

void handelBossRobot(Entity entity, float elapsed_ms);
void handleSpiderRobot(Entity entity, float elapsed_ms);
bool wall_hit(Motion start, Motion end) {
	std::pair<int, int> end_p = translate_vec2(end);
	std::pair<int, int> start_p = translate_vec2(start);

	int step_x = 0;
	int step_y = 0;

	if (end_p.first > start_p.first) {
		step_x = 1;
	}
	else {
		step_x = -1;
	}

	if (end_p.second > start_p.second) {
		step_y = 1;
	}
	else {
		step_y = -1;
	}


	Entity T = registry.maps.entities[0];
	T_map m = registry.maps.get(T);

	if (m.tile_map[start_p.first][start_p.second] != 0) {
		return true;
	}

	while (end_p != start_p) {
		int delta_x = abs(end_p.first - start_p.first);
		int delta_y = abs(end_p.second - start_p.second);
		if (delta_x > delta_y) {
			start_p.first += step_x;
		}
		else {
			start_p.second += step_y;
		}
		if (m.tile_map[start_p.first][start_p.second] != 0) {
			return true;
		}
	}
	return false;
}

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
		Robot r = registry.robots.get(e);
		if (e.id != entity.id) {
			if (length(companion.position - m.position) < curr_min && !r.companion) {
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

	if (registry.robotAnimations.has(entity)) {
		RobotAnimation& ra = registry.robotAnimations.get(entity);

		for (Entity boss_entity : registry.bossRobots.entities) {
			Motion& boss_motion = registry.motions.get(boss_entity);
			float distance_to_boss = glm::length(boss_motion.position - motion.position);

			if (distance_to_boss < 384.f) {
				motion.velocity = vec2(0);

				if (ra.current_state != RobotState::ATTACK) {
					ra.setState(RobotState::ATTACK, ra.current_dir);
				}
				else if (ra.current_frame == ra.getMaxFrames() - 1) {
					ra.current_frame = 0;

					Robot& ro = registry.robots.get(entity);
					vec2 target_velocity = normalize((boss_motion.position - motion.position)) * 225.f;
					vec2 temp = motion.position - boss_motion.position;
					float angle = atan2(temp.y, temp.x);
					angle += 3.14;
					createProjectile(motion.position, target_velocity, angle, ro.ice_proj, true);
				}

				attacking = true;
				break;
			}
		}

		if (!attacking && temp.dist < 384.f && temp.i.id != entity.id && ra.current_state != RobotState::DEAD) {
			motion.velocity = vec2(0);

			if (ra.current_state != RobotState::ATTACK) {
				ra.setState(RobotState::ATTACK, ra.current_dir);
			}
			else if (ra.current_frame == ra.getMaxFrames() - 1) {
				ra.current_frame = 0;

				Robot& ro = registry.robots.get(entity);

				Motion& enemy_motion = registry.motions.get(temp.i);
				vec2 target_velocity = normalize((enemy_motion.position - motion.position)) * 225.f;
				vec2 temp = motion.position - enemy_motion.position;
				float angle = atan2(temp.y, temp.x);
				angle += 3.14;
				createProjectile(motion.position, target_velocity, angle, ro.ice_proj, true);
			}

			attacking = true;
		}

		if (!attacking && ra.current_state != RobotState::DEAD) {
			Direction a = a_star_ai(motion);
			ra.setState(RobotState::WALK, a);
		}

		Robot& ro = registry.robots.get(entity);
		if (ro.current_health <= 0) {
			if (!ro.should_die) {
				ro.should_die = true;
				ra.setState(RobotState::DEAD, ra.current_dir);
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

	if (registry.iceRobotAnimations.has(entity)) {
		IceRobotAnimation& ra = registry.iceRobotAnimations.get(entity);

		for (Entity boss_entity : registry.bossRobots.entities) {
			Motion& boss_motion = registry.motions.get(boss_entity);
			float distance_to_boss = glm::length(boss_motion.position - motion.position);

			if (distance_to_boss < 384.f) {
				motion.velocity = vec2(0);

				if (ra.current_state != IceRobotState::ATTACK) {
					ra.setState(IceRobotState::ATTACK, ra.current_dir);
				}
				else if (ra.current_frame == ra.getMaxFrames() - 1) {
					ra.current_frame = 0;

					Robot& ro = registry.robots.get(entity);
					vec2 target_velocity = normalize((boss_motion.position - motion.position)) * 225.f;
					vec2 temp = motion.position - boss_motion.position;
					float angle = atan2(temp.y, temp.x);
					angle += 3.14;
					createProjectile(motion.position, target_velocity, angle, ro.ice_proj, true);
				}

				attacking = true;
				break;
			}
		}

		if (!attacking && temp.dist < 384.f && temp.i.id != entity.id && ra.current_state != IceRobotState::DEAD) {
			motion.velocity = vec2(0);

			if (ra.current_state != IceRobotState::ATTACK) {
				ra.setState(IceRobotState::ATTACK, ra.current_dir);
			}
			else if (ra.current_frame == ra.getMaxFrames() - 1) {
				ra.current_frame = 0;

				Robot& ro = registry.robots.get(entity);

				Motion& enemy_motion = registry.motions.get(temp.i);
				vec2 target_velocity = normalize((enemy_motion.position - motion.position)) * 225.f;
				vec2 temp = motion.position - enemy_motion.position;
				float angle = atan2(temp.y, temp.x);
				angle += 3.14;
				createProjectile(motion.position, target_velocity, angle, ro.ice_proj, true);
			}

			attacking = true;
		}

		if (!attacking && ra.current_state != IceRobotState::DEAD) {
			Direction a = a_star_ai(motion);
			ra.setState(IceRobotState::WALK, a);
		}

		Robot& ro = registry.robots.get(entity);
		if (ro.current_health <= 0) {
			if (!ro.should_die) {
				ro.should_die = true;
				ra.setState(IceRobotState::DEAD, ra.current_dir);
				ro.death_cd = ra.getMaxFrames() * ra.FRAME_TIME * 1000.f;
			}
			else {
				ro.death_cd -= elapsed_ms;
				if (ro.death_cd < 0) {
					registry.remove_all_components_of(entity);
				}
			}
		}
		return;
	}
}


void handelRobot(Entity entity, float elapsed_ms) {
	Motion& motion = registry.motions.get(entity);
	Entity player = registry.players.entities[0];
	Motion& player_motion = registry.motions.get(player);
	if (registry.robotAnimations.has(entity)) {
		RobotAnimation& ra = registry.robotAnimations.get(entity);

		Robot& ro = registry.robots.get(entity);
		if (ro.companion) {
			handelCompanion(entity, elapsed_ms);
			return;
		}

		if (shouldmv(entity) && registry.robotAnimations.get(entity).current_state != RobotState::DEAD) {
			Direction a = a_star_ai(motion);
			RobotAnimation& ra = registry.robotAnimations.get(entity);
			ra.setState(RobotState::WALK, a);
		}

		if (shouldattack(entity) && registry.robotAnimations.get(entity).current_state != RobotState::DEAD) {
			RobotAnimation& ra = registry.robotAnimations.get(entity);
			motion.velocity = vec2(0);
			if (wall_hit(motion, player_motion)) {
				Direction a = a_star_ai(motion);
				RobotAnimation& ra = registry.robotAnimations.get(entity);
				ra.setState(RobotState::WALK, a);
			}
			else if (ra.current_state != RobotState::ATTACK) {
				ra.setState(RobotState::ATTACK, ra.current_dir);
			}
			else if (ra.current_frame == ra.getMaxFrames() - 1) {
				ra.current_frame = 0;
				Robot& ro = registry.robots.get(entity);
				//std::cout << "fire shot" << std::endl;
				Entity player = registry.players.entities[0];
				Motion& player_motion = registry.motions.get(player);
				vec2 target_velocity = normalize((player_motion.position - motion.position)) * 225.f;
				vec2 temp = motion.position - player_motion.position;
				float angle = atan2(temp.y, temp.x);
				angle += 3.14;
				createProjectile(motion.position, target_velocity, angle, ro.ice_proj, false);
				//ro.ice_proj = !ro.ice_proj;
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
						ro.current_health = ro.max_health/2;
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
		return;
	}
	
	if (registry.iceRobotAnimations.has(entity)) {
		IceRobotAnimation& ra = registry.iceRobotAnimations.get(entity);

		Robot& ro = registry.robots.get(entity);
		if (ro.companion) {
			handelCompanion(entity, elapsed_ms);
			return;
		}

		if (shouldmv(entity) && registry.iceRobotAnimations.get(entity).current_state != IceRobotState::DEAD) {
			Direction a = a_star_ai(motion);
			IceRobotAnimation& ra = registry.iceRobotAnimations.get(entity);
			ra.setState(IceRobotState::WALK, a);
		}

		if (shouldattack(entity) && registry.iceRobotAnimations.get(entity).current_state != IceRobotState::DEAD) {
			IceRobotAnimation& ra = registry.iceRobotAnimations.get(entity);
			motion.velocity = vec2(0);
			if (wall_hit(motion, player_motion)) {
				Direction a = a_star_ai(motion);
				RobotAnimation& ra = registry.robotAnimations.get(entity);
				ra.setState(RobotState::WALK, a);
			}
			else if (ra.current_state != IceRobotState::ATTACK) {
				ra.setState(IceRobotState::ATTACK, ra.current_dir);
			}
			else if (ra.current_frame == 8) {
				ra.current_frame++;
				Robot& ro = registry.robots.get(entity);
				Entity player = registry.players.entities[0];
				Motion& player_motion = registry.motions.get(player);
				vec2 target_velocity = normalize((player_motion.position - motion.position)) * 225.f;
				vec2 temp = motion.position - player_motion.position;
				float angle = atan2(temp.y, temp.x);
				angle += 3.14;
				createProjectile(motion.position, target_velocity, angle, ro.ice_proj, false);
			}
			else if (ra.current_frame == ra.getMaxFrames() - 1) {
				ra.current_frame = 0;
			}
		}


		if (shouldidle(entity) && registry.iceRobotAnimations.get(entity).current_state != IceRobotState::DEAD) {
			motion.velocity = vec2(0);
			ra.setState(IceRobotState::IDLE, ra.current_dir);
		}
		if (registry.robots.has(entity)) {
			Robot& ro = registry.robots.get(entity);
			if (ro.current_health <= 0) {
				if (!ro.should_die) {
					ro.should_die = true;
					ra.setState(IceRobotState::DEAD, ra.current_dir);
					ro.death_cd = ra.getMaxFrames() * ra.FRAME_TIME * 1000.f;
					if (ro.isCapturable) {
						ro.showCaptureUI = true;
						ro.current_health = ro.max_health/2;
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
		return;
	}
}

void handelBossRobot(Entity entity, float elapsed_ms) {
    Motion& motion = registry.motions.get(entity);
    BossRobotAnimation& ra = registry.bossRobotAnimations.get(entity);

    static float shoot_timer = 0.0f;
    const float shoot_interval = 2.0f;
    Entity player = registry.players.entities[0];
    Motion& player_motion = registry.motions.get(player);

    static int projectile_count = 0;
    const int max_projectiles = 5;
    static float dash_timer = 0.0f;
    const float dash_duration = 4.0f;
    const float dash_speed = 200.0f;

    Entity target_entity = player;
    Motion* target_motion = &player_motion;
    float closest_distance = glm::distance(motion.position, player_motion.position);

    for (Entity companion_entity : registry.robots.entities) {
        Robot& companion_robot = registry.robots.get(companion_entity);
        if (companion_robot.companion) {
            Motion& companion_motion = registry.motions.get(companion_entity);
            float companion_distance = glm::distance(motion.position, companion_motion.position);

            if (companion_distance < closest_distance) {
                target_entity = companion_entity;
                target_motion = &companion_motion;
                closest_distance = companion_distance;
            }
        }
    }

    if (bossShouldattack(entity)) {
        motion.velocity = vec2(0);

        if (ra.current_state != BossRobotState::ATTACK) {
            ra.setState(BossRobotState::ATTACK, ra.current_dir);
            shoot_timer = 0.0f;
        } else {
            shoot_timer += elapsed_ms / 1000.0f;
            if (shoot_timer >= shoot_interval) {
                shoot_timer = 0.0f;

                // Fire projectiles
                vec2 central_velocity = normalize(target_motion->position - motion.position) * 185.0f;
                createBossProjectile(motion.position, central_velocity, atan2(central_velocity.y, central_velocity.x), 10);

                for (int i = -3; i <= 3; ++i) {
                    if (i == 0) continue;
                    float angle_offset = i * glm::radians(15.0f);
                    vec2 target_velocity = normalize(target_motion->position - motion.position);

                    float cos_angle = cos(angle_offset);
                    float sin_angle = sin(angle_offset);
                    vec2 rotated_velocity = vec2(
                        target_velocity.x * cos_angle - target_velocity.y * sin_angle,
                        target_velocity.x * sin_angle + target_velocity.y * cos_angle
                    );

                    target_velocity = rotated_velocity * 185.0f;
                    createBossProjectile(motion.position, target_velocity, atan2(target_velocity.y, target_velocity.x), 10);
                }

                projectile_count++;
                if (projectile_count >= max_projectiles) {
                    projectile_count = 0;
                    dash_timer = 0.0f;
                }
            }
        }
    } else if (bossShouldmv(entity)) {
        ra.setState(BossRobotState::WALK, ra.current_dir);
        motion.velocity = normalize(player_motion.position - motion.position) * 50.0f;
    } else {
        ra.setState(BossRobotState::IDLE, ra.current_dir);
    }

    // Handle dash attack
    if (dash_timer >= 0.0f) {
        motion.velocity = normalize(player_motion.position - motion.position) * dash_speed;
        dash_timer += elapsed_ms / 1000.0f;

        // Check for collision with player
        if (collides(motion, player_motion)) {
            Player& pl = registry.players.get(player);
            pl.current_health -= 30;
            dash_timer = -1.0f;
            motion.velocity = vec2(0);
        }

        // Stop dashing after 4 seconds
        if (dash_timer >= dash_duration) {
            dash_timer = -1.0f; 
            motion.velocity = vec2(0);
        }
    }

    ra.update(elapsed_ms);

    if (registry.bossRobots.has(entity)) {
        BossRobot& ro = registry.bossRobots.get(entity);
        if (ro.current_health <= 0) {
            if (!ro.should_die) {
                ro.should_die = true;
                ro.death_cd = ra.getMaxFrames() * ra.FRAME_TIME * 1000.f;
            } else {
                ro.death_cd -= elapsed_ms;
                if (ro.death_cd < 0) {
                    registry.remove_all_components_of(entity);
                }
            }
        }
    }
}

void handleSpiderRobot(Entity entity, float elapsed_ms) {
	Motion& motion = registry.motions.get(entity);
	SpiderRobotAnimation& ra = registry.spiderRobotAnimations.get(entity);

	const float attack_range = 50.0f;
	const float patrol_speed = 100.0f;
	const float follow_speed = 50.0f;
	const float direction_change_interval = 3.0f;

	Entity player = registry.players.entities[0];
	Motion& player_motion = registry.motions.get(player);

	float distance_to_player = glm::distance(motion.position, player_motion.position);

	if (distance_to_player <= attack_range) {
		motion.velocity = vec2(0);
		if (ra.current_state != SpiderRobotState::ATTACK) {
			ra.setState(SpiderRobotState::ATTACK, ra.current_dir);
		}
	}
	else if (spiderShouldmv(entity)) {
		Direction direction = bfs_ai(motion);
		ra.setState(SpiderRobotState::WALK, direction);
		motion.velocity = normalize(player_motion.position - motion.position) * follow_speed;
	}
	else {
		static float patrol_timer = 0.0f;
		patrol_timer += elapsed_ms / 1000.0f;

		if (patrol_timer >= direction_change_interval) {
			int random_dir = rand() % 4;
			Direction random_direction = static_cast<Direction>(random_dir);
			ra.setState(SpiderRobotState::WALK, random_direction);

			switch (random_direction) {
			case Direction::UP:
				motion.velocity = vec2(0, -patrol_speed);
				break;
			case Direction::DOWN:
				motion.velocity = vec2(0, patrol_speed);
				break;
			case Direction::LEFT:
				motion.velocity = vec2(-patrol_speed, 0);
				break;
			case Direction::RIGHT:
				motion.velocity = vec2(patrol_speed, 0);
				break;
			}
			patrol_timer = 0.0f;
		}

		if (glm::length(motion.velocity) < 0.1f) {
			ra.setState(SpiderRobotState::IDLE, Direction::DOWN);
		}
	}
	if (registry.spiderRobots.has(entity)) {
		SpiderRobot& ro = registry.spiderRobots.get(entity);
		if (ro.current_health <= 0) {
			if (!ro.should_die) {
				ro.should_die = true;
				ro.death_cd = ra.getMaxFrames() * ra.FRAME_TIME * 1000.f;
			}
			else {
				ro.death_cd -= elapsed_ms;
				if (ro.death_cd < 0) {
					Player& p = registry.players.get(player);
					p.inventory.addItem("Robot Parts", 1);
					registry.remove_all_components_of(entity);
				}
			}
		}
	}

	ra.update(elapsed_ms);
}


void PhysicsSystem::step(float elapsed_ms, WorldSystem* world)
{

	ComponentContainer<Motion>& motion_container = registry.motions;
	// Move entities based on the time passed, ensuring entities move at consistent speeds
	std::vector<Entity> should_remove;
	auto& motion_registry = registry.motions;
	for (Entity entity : registry.spiderRobots.entities) {
		SpiderRobot& spider = registry.spiderRobots.get(entity);
		if (spider.attack_timer > 0.0f) {
			spider.attack_timer -= elapsed_ms / 1000.0f;
		}
	}
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
		if (registry.spiderRobots.has(entity)) {
			handleSpiderRobot(entity, elapsed_ms);
		//	printf("spidercreated here");
			//handelBossRobot(entity, elapsed_ms);

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


		if (registry.projectile.has(entity) || registry.bossProjectile.has(entity)) {
			if (motion.position.x < 0.0f || motion.position.x > map_width * 64.f ||
				motion.position.y < 0.0f || motion.position.y > map_height * 64.f) {
				registry.remove_all_components_of(entity);
				printf("entity removed");
				continue;
			}
		}
		
		if (registry.bossProjectile.has(entity)) {
			bossProjectile& proj = registry.bossProjectile.get(entity);
			// Update the time variable for sine wave calculation
			proj.time += elapsed_ms / 1000.0f;
			// Calculate the new vertical position based on the sine wave
			float sine_offset = proj.amplitude * sin(proj.frequency * proj.time);
			motion.position.y += sine_offset;
			
			// Check for out-of-bounds and remove if necessary
			if (motion.position.x < 0.0f || motion.position.x > map_width * 64.f ||
			motion.position.y < 0.0f || motion.position.y > map_height * 64.f) {
				registry.remove_all_components_of(entity);
				printf("Boss projectile removed");
				continue;
			}

			// companion robot dmg
			/*for (Entity robot_entity : registry.robots.entities) {
				Robot& robot = registry.robots.get(robot_entity);
				if (robot.companion && robot.showCaptureUI) {
					continue;
				}
				if (robot.companion) { 
					Motion& robot_motion = registry.motions.get(robot_entity);

					if (collides(motion, robot_motion)) {
						robot.current_health -= proj.dmg; 
						registry.remove_all_components_of(entity);
						if (robot.current_health <= 0) {
							robot.should_die = true;
						}
						break; 
					}
				}
			}*/
		}

		if (registry.robots.has(entity) || registry.players.has(entity)) {
			attackbox_check(entity);
		}
		if (registry.spiderRobots.has(entity) || registry.players.has(entity)) {
			attackbox_check(entity);
		}
		if (registry.bossRobots.has(entity) || registry.players.has(entity)) {
			attackbox_check(entity);
		}

		if (!registry.tiles.has(entity) && !registry.robots.has(entity) && !registry.bossRobots.has(entity)) {

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
							if (registry.projectile.has(entity) || registry.bossProjectile.has(entity)) {
								flag = false;
								//should_remove.push_back(entity_j);
								should_remove.push_back(entity);
							}
						}
					}


				}
			}

			if (!registry.bossProjectile.has(entity) && !registry.projectile.has(entity)) {
				bound_check(motion);
			}
		}

		/*if (!registry.tiles.has(entity) && !registry.bossRobots.has(entity)) {

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
							if (registry.bossProjectile.has(entity)) {
								flag = false;
								should_remove.push_back(entity);
							}
						}
					}

				
				}
			}

			if (!registry.bossProjectile.has(entity) && !registry.projectile.has(entity)) {
				bound_check(motion);
			}
		}*/

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
				static bool notification_active = false;
				if (registry.doors.has(entity_j)) {
					Door& door = registry.doors.get(entity_j);

					if (door.is_locked || !door.is_open) {
						// Block the player's motion
						motion_i.position -= motion_i.velocity * (elapsed_ms / 1000.f);
						motion_i.velocity = vec2(0);

						// Check if a notification can be displayed
						//if (!door.notification_active && door.in_range) {
						//	static std::vector<std::string> messages = {
						//		"Hmm, it's locked.",
						//		"Seems like I need a keycard to open this.",
						//		"This door won't budge.",
						//		"Looks like I can't get through without a keycard.",
						//		"Locked. I need a keycard. Maybe one of these robots would have it."
						//	};
						//	std::random_device rd;
						//	std::mt19937 rng(rd());
						//	std::uniform_int_distribution<int> dist(0, messages.size() - 1);

						//	std::string message = messages[dist(rng)];
						//	createNotification(message, 4.0f);

						//	door.notification_active = true; // Mark the notification as active for this door
						//}
						//else if (!door.in_range) {
						//	// Reset the state when out of range
						//	door.notification_active = false;
						//}

					}

					if (registry.projectile.has(entity_i)) {
						registry.remove_all_components_of(entity_i);
					}
				}

				// Reset notification_active after all notifications are processed
				if (registry.notifications.entities.empty()) {
					notification_active = false; // Allow new notifications
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


Direction a_star_ai(Motion& mo) {
    Entity player = registry.players.entities[0];
    Motion& player_motion = registry.motions.get(player);

    Entity T = registry.maps.entities[0];
    T_map m = registry.maps.get(T);

    std::pair<int, int> end = translate_vec2(player_motion);
    std::pair<int, int> start = translate_vec2(mo);

	if (start.first < 0 || start.second < 0 || end.first < 0 || end.second < 0 ||
		start.first >= m.tile_map.size() || start.second >= m.tile_map[0].size() ||
		end.first >= m.tile_map.size() || end.second >= m.tile_map[0].size()) {
		std::cerr << "Invalid start/end position for a_star.\n";
		mo.velocity = vec2(0); 
		return Direction::LEFT; 
	}


    std::vector<std::pair<int, int>> path = a_star(m.tile_map, start, end);
    if (!path.empty()) {
        vec2 target = translate_pair(path[1]);
        mo.velocity = normalize(vec2(target.x + 32, target.y + 32) - mo.position) * 64.f;

        if (start.first > path[1].first) {
            return Direction::UP;
        }
        if (start.first < path[1].first) {
            return Direction::DOWN;
        }
        if (start.second < path[1].second) {
            return Direction::RIGHT;
        }
        if (start.second > path[1].second) {
            return Direction::LEFT;
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

std::vector<std::pair<int, int>> a_star(const std::vector<std::vector<int>>& tile_map, 
        std::pair<int, int> start, std::pair<int, int> end) {
    int rows = tile_map.size();
    int cols = tile_map[0].size();

    // Priority queue for the open set
    using Pair = std::pair<float, std::pair<int, int>>; // (f_cost, position)
    std::priority_queue<Pair, std::vector<Pair>, std::greater<Pair>> open_set;

    // tracking costs and paths
    std::map<std::pair<int, int>, std::pair<int, int>> came_from;
    std::map<std::pair<int, int>, float> g_costs;

    open_set.push({0, start});
    g_costs[start] = 0;

    int dirX[4] = {0, 0, -1, 1};
    int dirY[4] = {-1, 1, 0, 0};

    while (!open_set.empty()) {
	// getting position
        auto current = open_set.top().second; 
        open_set.pop();

        if (current == end) {
            // Reconstruct the path
            std::vector<std::pair<int, int>> path;
            for (auto at = end; at != start; at = came_from[at]) {
                path.push_back(at);
            }
            path.push_back(start);
            std::reverse(path.begin(), path.end());
            return path;
        }

        for (int i = 0; i < 4; ++i) {
            int x = current.first + dirX[i];
            int y = current.second + dirY[i];

            if (x >= 0 && x < rows && y >= 0 && y < cols && tile_map[x][y] == 0) {
                float tentative_g_cost = g_costs[current] + 1; // Cost to move to adjacent node
                std::pair<int, int> neighbor = {x, y};

                if (g_costs.find(neighbor) == g_costs.end() || tentative_g_cost < g_costs[neighbor]) {
                    came_from[neighbor] = current;
                    g_costs[neighbor] = tentative_g_cost;
                    float h_cost = static_cast<float>(std::hypot(end.first - x, end.second - y));
                    open_set.push({tentative_g_cost + h_cost, neighbor});
                }
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
				if (!ro.companion && ro.showCaptureUI) {
					continue;
				}
				if (!ro.companion) {
					ro.current_health -= attack_i.dmg;
				}
			}

			if (registry.bossRobots.has(en) && attack_i.friendly) {
				BossRobot& ro = registry.bossRobots.get(en);
				ro.current_health -= attack_i.dmg;
			}
			if (registry.spiderRobots.has(en) && attack_i.friendly) {
				SpiderRobot& ro = registry.spiderRobots.get(en);
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


bool attack_hit(const Motion& motion1, const attackBox& motion2) {
    vec2 size1 = get_bounding_box(motion1);
    vec2 size2 = { abs(motion2.bb.x), abs(motion2.bb.y) };

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
bool spiderShouldmv(Entity e) {
	SpiderRobot r = registry.spiderRobots.get(e);
	Motion m = registry.motions.get(e);

	Entity pl = registry.players.entities[0];
	Motion plm = registry.motions.get(pl);
	return inbox(plm, r.search_box, m.position) && !inbox(plm, r.attack_box, m.position) && !inbox(plm, r.panic_box, m.position);
}
bool bossShouldattack(Entity e) {
    BossRobot r = registry.bossRobots.get(e);
    Motion m = registry.motions.get(e);

    Entity pl = registry.players.entities[0];
    Motion plm = registry.motions.get(pl);
    
    float attack_range = 600;
	bool player_in_range = glm::distance(plm.position, m.position) <= attack_range;

	for (Entity companion_entity : registry.robots.entities) {
		Robot& companion_robot = registry.robots.get(companion_entity);
		if (companion_robot.companion) {
			Motion& companion_motion = registry.motions.get(companion_entity);
			if (glm::distance(companion_motion.position, m.position) <= attack_range) {
				return true;
			}
		}
	}

	return player_in_range;
}


bool spiderShouldattack(Entity e) {
	SpiderRobot r = registry.spiderRobots.get(e);
	Motion m = registry.motions.get(e);

	Entity pl = registry.players.entities[0];
	Motion plm = registry.motions.get(pl);

	float attack_range = 600;
	bool player_in_range = glm::distance(plm.position, m.position) <= attack_range;

	for (Entity companion_entity : registry.robots.entities) {
		Robot& companion_robot = registry.robots.get(companion_entity);
		if (companion_robot.companion) {
			Motion& companion_motion = registry.motions.get(companion_entity);
			if (glm::distance(companion_motion.position, m.position) <= attack_range) {
				return true;
			}
		}
	}

	return player_in_range;
}
bool bossShouldidle(Entity e) {
	BossRobot r = registry.bossRobots.get(e);
	Motion m = registry.motions.get(e);

	Entity pl = registry.players.entities[0];
	Motion plm = registry.motions.get(pl);
	return inbox(plm, r.panic_box, m.position);
}
