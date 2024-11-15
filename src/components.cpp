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
		{"slow_count_down", player.slow_count_down}
	};
}

void from_json(const json& j, Player& player) {
	j.at("inventory").get_to(player.inventory);  
	j.at("speed").get_to(player.speed);
	j.at("current_health").get_to(player.current_health);
	j.at("max_health").get_to(player.max_health);
	j.at("slow").get_to(player.slow);
	j.at("slow_count_down").get_to(player.slow_count_down);
}

void to_json(nlohmann::json& j, const BaseAnimation& anim) {
	j = nlohmann::json{
		{"sprite_size", anim.sprite_size},
		{"s_width", anim.s_width},
		{"s_height", anim.s_height},
		{"current_frame_time", anim.current_frame_time},
		{"current_frame", anim.current_frame},
		{"current_dir", anim.current_dir}
	};
}

void from_json(const nlohmann::json& j, BaseAnimation& anim) {
	j.at("sprite_size").get_to(anim.sprite_size);
	j.at("s_width").get_to(anim.s_width);
	j.at("s_height").get_to(anim.s_height);
	j.at("current_frame_time").get_to(anim.current_frame_time);
	j.at("current_frame").get_to(anim.current_frame);
	j.at("current_dir").get_to(anim.current_dir);
}

void to_json(nlohmann::json& j, const PlayerAnimation& anim) {
	to_json(j, static_cast<const BaseAnimation&>(anim)); 
	j["is_walking"] = anim.is_walking;
	j["can_attack"] = anim.can_attack;
}

// from_json for PlayerAnimation
void from_json(const nlohmann::json& j, PlayerAnimation& anim) {
	from_json(j, static_cast<BaseAnimation&>(anim)); 
	j.at("is_walking").get_to(anim.is_walking);
	j.at("can_attack").get_to(anim.can_attack);
}