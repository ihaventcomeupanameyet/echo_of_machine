#include "components.hpp"
#include "render_system.hpp" // for gl_has_errors

#define STB_IMAGE_IMPLEMENTATION
#include "../ext/stb_image/stb_image.h"

// stlib
#include <iostream>
#include <sstream>

Debug debugging;
float death_timer_counter_ms = 3000;

// Very, VERY simple OBJ loader from https://github.com/opengl-tutorials/ogl tutorial 7
// (modified to also read vertex color and omit uv and normals)
bool Mesh::loadFromOBJFile(std::string obj_path, std::vector<ColoredVertex>& out_vertices, std::vector<uint16_t>& out_vertex_indices, vec2& out_size)
{
	// disable warnings about fscanf and fopen on Windows
#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

	printf("Loading OBJ file %s...\n", obj_path.c_str());
	// Note, normal and UV indices are not loaded/used, but code is commented to do so
	std::vector<uint16_t> out_uv_indices, out_normal_indices;
	std::vector<glm::vec2> out_uvs;
	std::vector<glm::vec3> out_normals;

	FILE* file = fopen(obj_path.c_str(), "r");
	if (file == NULL) {
		std::cerr << "Impossible to open the file ! Are you in the right path ? See Tutorial 1 for details" << std::endl;
		getchar();
		return false;
	}

	while (1) {
		char lineHeader[128];
		// read the first word of the line
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break; // EOF = End Of File. Quit the loop.

		if (strcmp(lineHeader, "v") == 0) {
			ColoredVertex vertex;
			int matches = fscanf(file, "%f %f %f %f %f %f\n", &vertex.position.x, &vertex.position.y, &vertex.position.z,
				&vertex.color.x, &vertex.color.y, &vertex.color.z);
			if (matches == 3)
				vertex.color = { 1,1,1 };
			out_vertices.push_back(vertex);
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			glm::vec2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y);
			uv.y = -uv.y; // Invert V coordinate since we will only use DDS texture, which are inverted. Remove if you want to use TGA or BMP loaders.
			out_uvs.push_back(uv);
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			out_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f") == 0) {
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], normalIndex[3], uvIndex[3];

			int matches = fscanf(file, "%d %d %d\n", &vertexIndex[0], &vertexIndex[1], &vertexIndex[2]);
			if (matches == 1) // try again
			{
				// Note first vertex index is already consumed by the first fscanf call (match ==1) since it aborts on the first error
				matches = fscanf(file, "//%d %d//%d %d//%d\n", &normalIndex[0], &vertexIndex[1], &normalIndex[1], &vertexIndex[2], &normalIndex[2]);
				if (matches != 5) // try again
				{
					matches = fscanf(file, "%d/%d %d/%d/%d %d/%d/%d\n", &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
					if (matches != 8)
					{
						printf("File can't be read by our simple parser :-( Try exporting with other options\n");
						fclose(file);
						return false;
					}
				}
			}

			// -1 since .obj starts counting at 1 and OpenGL starts at 0
			out_vertex_indices.push_back((uint16_t)vertexIndex[0] - 1);
			out_vertex_indices.push_back((uint16_t)vertexIndex[1] - 1);
			out_vertex_indices.push_back((uint16_t)vertexIndex[2] - 1);
			//out_uv_indices.push_back(uvIndex[0] - 1);
			//out_uv_indices.push_back(uvIndex[1] - 1);
			//out_uv_indices.push_back(uvIndex[2] - 1);
			out_normal_indices.push_back((uint16_t)normalIndex[0] - 1);
			out_normal_indices.push_back((uint16_t)normalIndex[1] - 1);
			out_normal_indices.push_back((uint16_t)normalIndex[2] - 1);
		}
		else {
			// Probably a comment, eat up the rest of the line
			char stupidBuffer[1000];
			fgets(stupidBuffer, 1000, file);
		}
	}
	fclose(file);

	// Compute bounds of the mesh
	vec3 max_position = { -99999,-99999,-99999 };
	vec3 min_position = { 99999,99999,99999 };
	for (ColoredVertex& pos : out_vertices)
	{
		max_position = glm::max(max_position, pos.position);
		min_position = glm::min(min_position, pos.position);
	}
	if(abs(max_position.z - min_position.z)<0.001)
		max_position.z = min_position.z+1; // don't scale z direction when everythin is on one plane

	vec3 size3d = max_position - min_position;
	out_size = size3d;

	// Normalize mesh to range -0.5 ... 0.5
	for (ColoredVertex& pos : out_vertices)
		pos.position = ((pos.position - min_position) / size3d) - vec3(0.5f, 0.5f, 0.5f);

	return true;
}


void to_json(json& j, const vec2& v) {
	j = json{ {"x", v.x}, {"y", v.y} };
}

void from_json(const json& j, vec2& v) {
	j.at("x").get_to(v.x);
	j.at("y").get_to(v.y);
}

void to_json(json& j, const attackBox& box) {
	json j1;
	to_json(j1, box.position);
	json j2;
	to_json(j2, box.bb);
	j = json{
		{"position", j1},
		{"bb",j2},
		{"dmg", box.dmg},
		{"friendly", box.friendly}
	};
}

void from_json(const nlohmann::json& j, attackBox& box) {
	from_json(j.at("position"), box.position);
	from_json(j.at("bb"), box.bb);
	j.at("dmg").get_to(box.dmg);
	j.at("friendly").get_to(box.friendly);
}

void to_json(json& j, const Motion& motion) {
	json position_json, velocity_json, target_velocity_json, scale_json, bb_json;

	to_json(position_json, motion.position);
	to_json(velocity_json, motion.velocity);
	to_json(target_velocity_json, motion.target_velocity);
	to_json(scale_json, motion.scale);
	to_json(bb_json, motion.bb);

	j = json{
		{"position", position_json},
		{"t_m", motion.t_m},
		{"start_angle", motion.start_angle},
		{"angle", motion.angle},
		{"end_angle", motion.end_engle},
		{"t", motion.t},
		{"should_rotate", motion.should_rotate},
		{"velocity", velocity_json},
		{"target_velocity", target_velocity_json},
		{"scale", scale_json},
		{"is_stuck", motion.is_stuck},
		{"bb", bb_json}
	};
}

void from_json(const json& j, Motion& motion) {
	from_json(j.at("position"), motion.position);
	j.at("t_m").get_to(motion.t_m);
	j.at("start_angle").get_to(motion.start_angle);
	j.at("angle").get_to(motion.angle);
	j.at("end_angle").get_to(motion.end_engle);
	j.at("t").get_to(motion.t);
	j.at("should_rotate").get_to(motion.should_rotate);
	from_json(j.at("velocity"), motion.velocity);
	from_json(j.at("target_velocity"), motion.target_velocity);
	from_json(j.at("scale"), motion.scale);
	j.at("is_stuck").get_to(motion.is_stuck);
	from_json(j.at("bb"), motion.bb);
}

void to_json(json& j, const DeathTimer& timer) {
	j = timer.counter_ms;
}

void from_json(const json& j, DeathTimer& timer) {
	j.get_to(timer.counter_ms);
}

void to_json(json& j, const Collision& collision) {
	j = json{ {"other", collision.other} };
}

void from_json(const json& j, Collision& collision) {
	j.at("other").get_to(collision.other);
}

void to_json(json& j, const Player& player) {
	j = json{
		{"inventory", player.inventory},  // Serialize Inventory
		{"speed", player.speed},
		{"current_health", player.current_health},
		{"max_health", player.max_health},
		{"slow", player.slow},
		{"slow_count_down", player.slow_count_down},
		{"armor_stat", player.armor_stat},
		{"weapon_stat", player.weapon_stat},
		{"current_stamina", player.current_stamina},
		{"max_stamina", player.max_stamina}
	};
}

void from_json(const json& j, Player& player) {
	j.at("inventory").get_to(player.inventory);  
	j.at("speed").get_to(player.speed);
	j.at("current_health").get_to(player.current_health);
	j.at("max_health").get_to(player.max_health);
	j.at("slow").get_to(player.slow);
	j.at("slow_count_down").get_to(player.slow_count_down);
	j.at("armor_stat").get_to(player.armor_stat);
	j.at("weapon_stat").get_to(player.weapon_stat);
	j.at("current_stamina").get_to(player.current_stamina);
	j.at("max_stamina").get_to(player.max_stamina);
}

void to_json(json& j, const BaseAnimation& anim) {
	j = json{
		{"sprite_size", anim.sprite_size},
		{"s_width", anim.s_width},
		{"s_height", anim.s_height},
		{"current_frame_time", anim.current_frame_time},
		{"current_frame", anim.current_frame},
		{"current_dir", anim.current_dir}
	};
}

void from_json(const json& j, BaseAnimation& anim) {
	j.at("sprite_size").get_to(anim.sprite_size);
	j.at("s_width").get_to(anim.s_width);
	j.at("s_height").get_to(anim.s_height);
	j.at("current_frame_time").get_to(anim.current_frame_time);
	j.at("current_frame").get_to(anim.current_frame);
	j.at("current_dir").get_to(anim.current_dir);
}

void to_json(json& j, const PlayerAnimation& anim) {
	to_json(j, static_cast<const BaseAnimation&>(anim)); 
	j["is_walking"] = anim.is_walking;
	j["can_attack"] = anim.can_attack;
}

void from_json(const json& j, PlayerAnimation& anim) {
	from_json(j, static_cast<BaseAnimation&>(anim)); 
	j.at("is_walking").get_to(anim.is_walking);
	j.at("can_attack").get_to(anim.can_attack);
}

void to_json(json& j, const RobotAnimation& anim) {
	to_json(j, static_cast<const BaseAnimation&>(anim));
	j["current_state"] = anim.current_state;
	j["current_dir"] = anim.current_dir;
	j["is_moving"] = anim.is_moving;
}

void from_json(const json& j, RobotAnimation& anim) {
	from_json(j, static_cast<BaseAnimation&>(anim));
	j.at("current_state").get_to(anim.current_state);
	j.at("current_dir").get_to(anim.current_dir);
	j.at("is_moving").get_to(anim.is_moving);
}

void to_json(json& j, const RenderRequest& request) {
	j = json{
		{"used_texture", static_cast<int>(request.used_texture)},
		{"used_effect", static_cast<int>(request.used_effect)},
		{"used_geometry", static_cast<int>(request.used_geometry)}
	};
}

void from_json(const json& j, RenderRequest& request) {
	request.used_texture = static_cast<TEXTURE_ASSET_ID>(j.at("used_texture").get<int>());
	request.used_effect = static_cast<EFFECT_ASSET_ID>(j.at("used_effect").get<int>());
	request.used_geometry = static_cast<GEOMETRY_BUFFER_ID>(j.at("used_geometry").get<int>());
}

void to_json(json& j, const ScreenState& state) {
	j = json{
		{"darken_screen_factor", state.darken_screen_factor},
		{"fade_in_factor", state.fade_in_factor},
		{"fade_in_progress", state.fade_in_progress},
		{"is_nighttime", state.is_nighttime},
		{"nighttime_factor", state.nighttime_factor}
	};
}

void from_json(const json& j, ScreenState& state) {
	j.at("darken_screen_factor").get_to(state.darken_screen_factor);
	j.at("fade_in_factor").get_to(state.fade_in_factor);
	j.at("fade_in_progress").get_to(state.fade_in_progress);
	j.at("is_nighttime").get_to(state.is_nighttime);
	j.at("nighttime_factor").get_to(state.nighttime_factor);
}

void to_json(json& j, const Robot& robot) {
	j = json{
		{"current_health", robot.current_health},
		{"max_health", robot.max_health},
		{"should_die", robot.should_die},
		{"death_cd", robot.death_cd},
		{"ice_proj", robot.ice_proj},
		{"isCapturable", robot.isCapturable},
		{"showCaptureUI", robot.showCaptureUI},
		{"speed", robot.speed},
		{"attack", robot.attack},
		{"max_speed", robot.max_speed},
		{"max_attack", robot.max_attack},
		{"search_box", {robot.search_box.x, robot.search_box.y}},
		{"attack_box", {robot.attack_box.x, robot.attack_box.y}},
		{"panic_box", {robot.panic_box.x, robot.panic_box.y}},
		{"disassembleItems", robot.disassembleItems}
	};
}

void from_json(const json& j, Robot& robot) {
	j.at("current_health").get_to(robot.current_health);
	j.at("max_health").get_to(robot.max_health);
	j.at("should_die").get_to(robot.should_die);
	j.at("death_cd").get_to(robot.death_cd);
	j.at("ice_proj").get_to(robot.ice_proj);
	j.at("isCapturable").get_to(robot.isCapturable);
	j.at("showCaptureUI").get_to(robot.showCaptureUI);
	j.at("speed").get_to(robot.speed);
	j.at("attack").get_to(robot.attack);
	j.at("max_speed").get_to(robot.max_speed);
	j.at("max_attack").get_to(robot.max_attack);
	robot.search_box = { j.at("search_box")[0], j.at("search_box")[1] };
	robot.attack_box = { j.at("attack_box")[0], j.at("attack_box")[1] };
	robot.panic_box = { j.at("panic_box")[0], j.at("panic_box")[1] };
	j.at("disassembleItems").get_to(robot.disassembleItems);
}

void to_json(json& j, const TileData& tileData) {
	j = json{
		{"top_left", {tileData.top_left.x, tileData.top_left.y}},
		{"bottom_right", {tileData.bottom_right.x, tileData.bottom_right.y}}
	};
}

void from_json(const json& j, TileData& tileData) {
	tileData.top_left = { j.at("top_left")[0], j.at("top_left")[1] };
	tileData.bottom_right = { j.at("bottom_right")[0], j.at("bottom_right")[1] };
}

void to_json(json& j, const Tile& tile) {
	j = json{
		{"tileset_id", tile.tileset_id},
		{"tile_id", tile.tile_id},
		{"walkable", tile.walkable},
		{"tile_data", tile.tile_data},
		{"atlas", static_cast<int>(tile.atlas)}
	};
}

void from_json(const json& j, Tile& tile) {
	j.at("tileset_id").get_to(tile.tileset_id);
	j.at("tile_id").get_to(tile.tile_id);
	j.at("walkable").get_to(tile.walkable);
	j.at("tile_data").get_to(tile.tile_data);

	int atlas_value;
	j.at("atlas").get_to(atlas_value);
	tile.atlas = static_cast<TEXTURE_ASSET_ID>(atlas_value);
}

void to_json(json& j, const TileSet& tileset) {
	j = json{
		{"map_width", tileset.map_width},
		{"map_height", tileset.map_height},
		{"tile_textures", tileset.tile_textures}
	};
}

void from_json(const json& j, TileSet& tileset) {
	j.at("map_width").get_to(tileset.map_width);
	j.at("map_height").get_to(tileset.map_height);
	j.at("tile_textures").get_to(tileset.tile_textures);
}

void to_json(json& j, const TileSetComponent& TileSetComponent) {
	j = json{
		{"tileset", TileSetComponent.tileset}
	};
}

void from_json(const json& j, TileSetComponent& TileSetComponent) {
	j.at("tileset").get_to(TileSetComponent.tileset);
}

void to_json(json& j, const Key& key) {
	j = json::object();
}

void from_json(const json& j, Key& key) {
}

void to_json(json& j, const ArmorPlate& ap) {
	j = json::object();
}

void from_json(const json& j, ArmorPlate& ap) {
}

void to_json(json& j, const Potion& ap) {
	j = json::object();
}

void from_json(const json& j, Potion& ap) {
}

void to_json(json& j, const DebugComponent& ap) {
	j = json::object();
}

void from_json(const json& j, DebugComponent& ap) {
}

void to_json(json& j, const T_map& t_map) {
	j = json{
		{"tile_map", t_map.tile_map},
		{"tile_size", t_map.tile_size}
	};
}

void from_json(const json& j, T_map& t_map) {
	j.at("tile_map").get_to(t_map.tile_map);
	j.at("tile_size").get_to(t_map.tile_size);
}

void to_json(json& j, const Spaceship& ap) {
	j = json::object();
}

void from_json(const json& j, Spaceship& ap) {
}

void to_json(nlohmann::json& j, const projectile& p) {
	j = nlohmann::json{
		{"dmg", p.dmg},
		{"ice", p.ice},
		{"friendly", p.friendly}
	};
}

void from_json(const nlohmann::json& j, projectile& p) {
	j.at("dmg").get_to(p.dmg);
	j.at("ice").get_to(p.ice);
	j.at("friendly").get_to(p.friendly);
}

void to_json(json& j, const IceRobotAnimation& animation) {
	j = static_cast<const BaseAnimation&>(animation);
	j["current_state"] = static_cast<int>(animation.current_state);
	j["current_dir"] = static_cast<int>(animation.current_dir);
	j["is_moving"] = animation.is_moving;
}

void from_json(const json& j, IceRobotAnimation& animation) {
	j.get_to(static_cast<BaseAnimation&>(animation));
	animation.current_state = static_cast<IceRobotState>(j.at("current_state").get<int>());
	animation.current_dir = static_cast<Direction>(j.at("current_dir").get<int>());
	animation.is_moving = j.at("is_moving").get<bool>();
}

void to_json(json& j, const BossRobotAnimation& animation) {
	j = static_cast<const BaseAnimation&>(animation);
	j["current_state"] = static_cast<int>(animation.current_state);
	j["current_dir"] = static_cast<int>(animation.current_dir);
}

void from_json(const json& j, BossRobotAnimation& animation) {
	j.get_to(static_cast<BaseAnimation&>(animation));
	animation.current_state = static_cast<BossRobotState>(j.at("current_state").get<int>());
	animation.current_dir = static_cast<Direction>(j.at("current_dir").get<int>());
}

void to_json(json& j, const bossProjectile& projectile) {
	j = json{
		{"dmg", projectile.dmg},
		{"amplitude", projectile.amplitude},
		{"frequency", projectile.frequency},
		{"time", projectile.time}
	};
}

void from_json(const json& j, bossProjectile& projectile) {
	j.at("dmg").get_to(projectile.dmg);
	j.at("amplitude").get_to(projectile.amplitude);
	j.at("frequency").get_to(projectile.frequency);
	j.at("time").get_to(projectile.time);
}

void to_json(json& j, const BossRobot& robot) {
	j = json{
		{"current_health", robot.current_health},
		{"max_health", robot.max_health},
		{"should_die", robot.should_die},
		{"death_cd", robot.death_cd},
		{"search_box", {robot.search_box.x, robot.search_box.y}},
		{"attack_box", {robot.attack_box.x, robot.attack_box.y}},
		{"panic_box", {robot.panic_box.x, robot.panic_box.y}}
	};
}

void from_json(const json& j, BossRobot& robot) {
	j.at("current_health").get_to(robot.current_health);
	j.at("max_health").get_to(robot.max_health);
	j.at("should_die").get_to(robot.should_die);
	j.at("death_cd").get_to(robot.death_cd);
	std::array<float, 2> box;
	j.at("search_box").get_to(box);
	robot.search_box = vec2{ box[0], box[1] };
	j.at("attack_box").get_to(box);
	robot.attack_box = vec2{ box[0], box[1] };
	j.at("panic_box").get_to(box);
	robot.panic_box = vec2{ box[0], box[1] };
}

void to_json(json& j, const Door& door) {
	j = json{
		{"is_right_door", door.is_right_door},
		{"is_locked", door.is_locked},
		{"is_open", door.is_open},
		{"in_range", door.in_range}
	};
}

void from_json(const json& j, Door& door) {
	j.at("is_right_door").get_to(door.is_right_door);
	j.at("is_locked").get_to(door.is_locked);
	j.at("is_open").get_to(door.is_open);
	j.at("in_range").get_to(door.in_range);
}

void to_json(json& j, const DoorAnimation& anim) {
	j = json{
		{"sprite_size", anim.sprite_size},
		{"s_width", anim.s_width},
		{"s_height", anim.s_height},
		{"current_frame_time", anim.current_frame_time},
		{"current_frame", anim.current_frame},
		{"is_opening", anim.is_opening}
	};
}

void from_json(const json& j, DoorAnimation& anim) {
	j.at("sprite_size").get_to(anim.sprite_size);
	j.at("s_width").get_to(anim.s_width);
	j.at("s_height").get_to(anim.s_height);
	j.at("current_frame_time").get_to(anim.current_frame_time);
	j.at("current_frame").get_to(anim.current_frame);
	j.at("is_opening").get_to(anim.is_opening);
}

void to_json(json& j, const Notification& n) {
	j = json{
		{"text", n.text},
		{"duration", n.duration},
		{"elapsed_time", n.elapsed_time},
		{"position", {n.position.x, n.position.y}},
		{"color", n.color},
		{"scale", n.scale}
	};
}

void from_json(const json& j, Notification& n) {
	j.at("text").get_to(n.text);
	j.at("duration").get_to(n.duration);
	j.at("elapsed_time").get_to(n.elapsed_time);
	n.position = { j.at("position")[0], j.at("position")[1] }; 
	j.at("color").get_to(n.color);
	j.at("scale").get_to(n.scale);
}


void to_json(json& j, const SpiderRobot& robot) {
	j = json{
		{"current_health", robot.current_health},
		{"max_health", robot.max_health},
		{"should_die", robot.should_die},
		{"death_cd", robot.death_cd},
		{"attack_timer", robot.attack_timer},
		{"attack_cooldown", robot.attack_cooldown},
		{"search_box", {robot.search_box.x, robot.search_box.y}},
		{"attack_box", {robot.attack_box.x, robot.attack_box.y}},
		{"panic_box", {robot.panic_box.x, robot.panic_box.y}}
	};
}
void from_json(const json& j, SpiderRobot& robot) {
	j.at("current_health").get_to(robot.current_health);
	j.at("max_health").get_to(robot.max_health);
	j.at("should_die").get_to(robot.should_die);
	j.at("death_cd").get_to(robot.death_cd);
	j.at("attack_timer").get_to(robot.attack_timer);
	j.at("attack_cooldown").get_to(robot.attack_cooldown);
	std::array<float, 2> box;
	j.at("search_box").get_to(box);
	robot.search_box = vec2{ box[0], box[1] };
	j.at("attack_box").get_to(box);
	robot.attack_box = vec2{ box[0], box[1] };
	j.at("panic_box").get_to(box);
	robot.panic_box = vec2{ box[0], box[1] };
}


void to_json(json& j, const SpiderRobotAnimation& animation) {
	j = static_cast<const BaseAnimation&>(animation);
	j["current_state"] = static_cast<int>(animation.current_state);
	j["current_dir"] = static_cast<int>(animation.current_dir);
}

void from_json(const json& j, SpiderRobotAnimation& animation) {
	j.get_to(static_cast<BaseAnimation&>(animation));
	animation.current_state = static_cast<SpiderRobotState>(j.at("current_state").get<int>());
	animation.current_dir = static_cast<Direction>(j.at("current_dir").get<int>());
}