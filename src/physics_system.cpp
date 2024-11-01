// internal
#include "physics_system.hpp"
#include "world_init.hpp"
#include "math_utils.hpp"
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

bool shouldattack(Entity e);

bool shouldidle(Entity e);

void handelRobot(Entity entity, float elapsed_ms);

// Returns the local bounding coordinates scaled by the current size of the entity
vec2 get_bounding_box(const Motion& motion)
{
	// abs is to avoid negative scale due to the facing direction.
	return { abs(motion.bb.x), abs(motion.bb.y) };
}



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


void handelRobot(Entity entity, float elapsed_ms) {
	Motion& motion = registry.motions.get(entity);
	RobotAnimation& ra = registry.robotAnimations.get(entity);
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
		else if (ra.current_frame == ra.getMaxFrames()-1){
			ra.current_frame = 0;
			std::cout << "fire shot" << std::endl;
			Entity player = registry.players.entities[0];
			Motion& player_motion = registry.motions.get(player);
			vec2 target_velocity = normalize((player_motion.position - motion.position)) * 30.f;
			vec2 temp = motion.position - player_motion.position;
			float angle = atan2(temp.y, temp.x);
			angle += 3.14;
			createProjectile(motion.position, target_velocity,angle,5);
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
	auto& motion_registry = registry.motions;
	for (uint i = 0; i < motion_registry.size(); i++)
	{
		Motion& motion = motion_registry.components[i];
		Entity entity = motion_registry.entities[i];
		float step_seconds = elapsed_ms / 1000.f;

		if (registry.tiles.has(entity)) {
			continue;
		}

		lerp_rotate(motion);

		if (registry.robots.has(entity)) {
			handelRobot(entity,elapsed_ms);
		}

		

		motion.velocity.x = linear_inter(motion.target_velocity.x, motion.velocity.x, step_seconds * 100.0f);
		motion.velocity.y = linear_inter(motion.target_velocity.y, motion.velocity.y, step_seconds * 100.0f);
		vec2 pos = motion.position;

		motion.position += motion.velocity * step_seconds;

		if (registry.robots.has(entity) || registry.players.has(entity)) {
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
								registry.remove_all_components_of(entity);
							}
						}
					}
				}
			}
			bound_check(motion);
		}
	}


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
			mo.velocity = normalize(vec2(target.x + 32, target.y + 32) - mo.position)*64.f;
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

		//std::cout << "att: " << attack_i.position.x << std::endl;

		if (attack_hit(mo, attack_i)) {
			if (registry.robots.has(en) && attack_i.friendly) {
				Robot& ro = registry.robots.get(en);
				ro.current_health -= attack_i.dmg;
			}
			if (registry.players.has(en) && !attack_i.friendly) {
				Player& pl = registry.players.get(en);
				pl.current_health -= attack_i.dmg;
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
	return inbox(plm,r.search_box,m.position)&&!inbox(plm, r.attack_box, m.position)&&!inbox(plm, r.panic_box, m.position);
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