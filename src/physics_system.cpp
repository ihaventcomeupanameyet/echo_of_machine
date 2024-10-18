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
void bfs_ai(Motion& mo);

float lerp_float(float start, float end, float t);

vec2 lerp_move(vec2 start, vec2 end, float t);

// Returns the local bounding coordinates scaled by the current size of the entity
vec2 get_bounding_box(const Motion& motion)
{
	// abs is to avoid negative scale due to the facing direction.
	return { abs(motion.scale.x), abs(motion.scale.y) };
}


// This is a SUPER APPROXIMATE check that puts a circle around the bounding boxes and sees
// if the center point of either object is inside the other's bounding-box-circle. You can
// surely implement a more accurate detection
bool collides(const Motion& motion1, const Motion& motion2)
{
	float scale_factor = 0.35;

	vec2 size1 = get_bounding_box(motion1) * scale_factor;
	vec2 size2 = get_bounding_box(motion2) * scale_factor;

	vec2 pos1_min = motion1.position - (size1 / 2.f);
	vec2 pos1_max = motion1.position + (size1 / 2.f);

	vec2 pos2_min = motion2.position - (size2 / 2.f);
	vec2 pos2_max = motion2.position + (size2 / 2.f);

	bool overlap_x = (pos1_min.x <= pos2_max.x && pos1_max.x >= pos2_min.x);
	bool overlap_y = (pos1_min.y <= pos2_max.y - 10 && pos1_max.y >= pos2_min.y - 35);

	return overlap_x && overlap_y;

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
		if (motion.t > 1.0f) {
			motion.t = 0;
			motion.should_rotate = false;
		}

		if (motion.t_m > 1.0f) {
			motion.t_m = 0;
			motion.should_move = false;
		}

		if (registry.robots.has(entity)) {
			if (!motion.should_move) {
				dumb_ai(motion);
			}
		}

		motion.velocity.x = linear_inter(motion.target_velocity.x, motion.velocity.x, step_seconds * 100.0f);
		motion.velocity.y = linear_inter(motion.target_velocity.y, motion.velocity.y, step_seconds * 100.0f);
		vec2 pos = motion.position;

		if (motion.should_move) {
			motion.position = lerp_move(motion.position_s, motion.position_e, motion.t_m);
			motion.t_m += 0.01;
		}

		motion.position += motion.velocity * step_seconds;

		if (motion.should_rotate) {
			motion.angle = lerp_float(motion.start_angle, motion.end_engle, motion.t);
			motion.t += 0.01;
		}

		if (!registry.tiles.has(entity) && !motion.should_move) {

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
							if (registry.robots.has(entity)) {
								bfs_ai(motion);
							}
							if (registry.players.has(entity)) {
								world->play_collision_sound();
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
}

void dumb_ai(Motion& mo) {
	Entity player = registry.players.entities[0];
	Motion& player_motion = registry.motions.get(player);
	mo.target_velocity = glm::normalize((player_motion.position - mo.position)) * vec2(50);
}

void bound_check(Motion& mo) {
	mo.position.x = max(min((float)window_width_px - 24, mo.position.x), 0.f + 24);
	mo.position.y = max(min((float)window_height_px - 48, mo.position.y), 0.f + 12);
}

void bfs_ai(Motion& mo) {
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
			mo.position_s = mo.position;
			mo.position_e = target;
			mo.should_move = true;
		}
		else {
			vec2 target = translate_pair(temp[0]);
			mo.position_s = mo.position;
			mo.position_e = player_motion.position;
			mo.should_move = true;
		}
		
	}
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