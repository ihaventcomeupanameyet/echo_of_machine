// internal
#include "render_system.hpp"
#include <SDL.h>
#include "tileset.hpp"
#include "inventory.hpp"
#include "tiny_ecs_registry.hpp"
#include "world_system.hpp"
// fonts
#include <ft2build.h>
#include FT_FREETYPE_H
#include <map>		
// matrices
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <assert.h>
#include <fstream>			// for ifstream
#include <sstream>			// for ostringstream
void RenderSystem::drawTexturedMesh(Entity entity, const mat3& projection)
{
	Motion& motion = registry.motions.get(entity);
	Transform transform;
	vec2 render_position = motion.position - camera_position;
	transform.translate(render_position);
	transform.rotate(motion.angle);
	transform.scale(motion.scale);
	// Apply scaling back to 1.0
	transform.scale(vec2(1.0f, 1.0f));

	if (registry.players.has(entity)) {
		transform.scale(vec2(1.20f, 1.20f));
	}

	if (debugging.in_debug_mode) {
		drawBoundingBox(entity, projection);
		drawReactionBox(entity, projection);
		drawBossReactionBox(entity, projection);
	}

	assert(registry.renderRequests.has(entity));
	const RenderRequest& render_request = registry.renderRequests.get(entity);

	// Select shader program
	const GLuint used_effect_enum = (GLuint)render_request.used_effect;
	assert(used_effect_enum != (GLuint)EFFECT_ASSET_ID::EFFECT_COUNT);
	const GLuint program = (GLuint)effects[used_effect_enum];

	// Use the selected shader program
	glUseProgram(program);
	gl_has_errors();

	// Set up the tile-specific texture and vertices
	if (render_request.used_texture == TEXTURE_ASSET_ID::TILE_ATLAS ||
		render_request.used_texture == TEXTURE_ASSET_ID::TILE_ATLAS_LEVELS) {

		assert(registry.tiles.has(entity));
		const Tile& tile = registry.tiles.get(entity);

		// Get the texture ID based on the tile atlas type
		GLuint texture_id = texture_gl_handles[(GLuint)tile.atlas];
		glBindTexture(GL_TEXTURE_2D, texture_id);
		gl_has_errors();

		// Get tile data for the tile atlas
		const TileSetComponent& tileset_component = registry.tilesets.get(registry.tilesets.entities[0]);
		const TileData& tile_data = tileset_component.tileset.getTileData(tile.tile_id);

		// Create vertices using tile's texture coordinates
		TexturedVertex vertices[4] = {
			{ vec3(-0.5f, 0.5f, -0.1f), tile_data.top_left },   // top-left
			{ vec3(0.5f, 0.5f, -0.1f), vec2(tile_data.bottom_right.x, tile_data.top_left.y) },  // top-right
			{ vec3(0.5f, -0.5f, -0.1f), tile_data.bottom_right },  // bottom-right
			{ vec3(-0.5f, -0.5f, -0.1f), vec2(tile_data.top_left.x, tile_data.bottom_right.y) }  // bottom-left
		};

		// Setup the VBO and IBO for the tiles
		if (!tile_vbo_initialized) {
			glGenBuffers(1, &tile_vbo);
			glGenBuffers(1, &tile_ibo);
			tile_vbo_initialized = true;
		}

		glBindBuffer(GL_ARRAY_BUFFER, tile_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
		gl_has_errors();

		// Define the tile indices (forming a square from two triangles)
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tile_ibo);
		static const uint16_t tile_indices[] = { 0, 1, 2, 2, 3, 0 }; // two triangles forming a square
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(tile_indices), tile_indices, GL_DYNAMIC_DRAW);
		gl_has_errors();
	}

	else if (render_request.used_effect == EFFECT_ASSET_ID::SPACESHIP) {
		GLuint program = effects[(GLuint)EFFECT_ASSET_ID::SPACESHIP];
		glUseProgram(program);
		gl_has_errors();

		const GLuint vbo = vertex_buffers[(GLuint)render_request.used_geometry];
		const GLuint ibo = index_buffers[(GLuint)render_request.used_geometry];
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		gl_has_errors();

		GLint in_position_loc = glGetAttribLocation(program, "in_position");
		GLint in_color = glGetAttribLocation(program, "in_color");

		glEnableVertexAttribArray(in_position_loc);
		glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
			sizeof(ColoredVertex), (void*)0);

		glEnableVertexAttribArray(in_color);
		glVertexAttribPointer(in_color, 3, GL_FLOAT, GL_FALSE,
			sizeof(ColoredVertex), (void*)sizeof(vec3));
		gl_has_errors();

		Transform transform;
		vec2 render_position = motion.position - camera_position;
		transform.translate(render_position);
		transform.rotate(motion.angle);
		transform.scale(motion.scale);

		GLint transform_loc = glGetUniformLocation(program, "transform");
		GLint projection_loc = glGetUniformLocation(program, "projection");
		GLint color_uloc = glGetUniformLocation(program, "fcolor");

		glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&transform.mat);
		glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection);

		vec3 color = registry.colors.has(entity) ? registry.colors.get(entity) : vec3(1.0f);
		glUniform3fv(color_uloc, 1, (float*)&color);
		gl_has_errors();

		GLsizei num_indices = 0;
		glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, (GLint*)&num_indices);
		num_indices /= sizeof(uint16_t);
		glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
		gl_has_errors();

		return;
	}


	
	else {
		// Render non-tile entities
		const GLuint vbo = vertex_buffers[(GLuint)render_request.used_geometry];
		const GLuint ibo = index_buffers[(GLuint)render_request.used_geometry];

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		gl_has_errors();


		if (registry.animations.has(entity) && render_request.used_texture == TEXTURE_ASSET_ID::PLAYER_FULLSHEET) {
			// Player with animation
			const auto& anim = registry.animations.get(entity);
			std::pair<vec2, vec2> coords = anim.getCurrentTexCoords();
			vec2 top_left = coords.first;
			vec2 bottom_right = coords.second;


			TexturedVertex vertices[4] = {
			{{-0.5f, +0.5f, 0.f}, {top_left.x, bottom_right.y}},
			{{+0.5f, +0.5f, 0.f}, {bottom_right.x, bottom_right.y}},
			{{+0.5f, -0.5f, 0.f}, {bottom_right.x, top_left.y}},
			{{-0.5f, -0.5f, 0.f}, {top_left.x, top_left.y}}
			};

			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
		}

		else if (registry.doorAnimations.has(entity) && (render_request.used_texture == TEXTURE_ASSET_ID::RIGHTDOORSHEET || render_request.used_texture == TEXTURE_ASSET_ID::BOTTOMDOORSHEET)) {
			const auto& anim = registry.doorAnimations.get(entity);
			std::pair<vec2, vec2> coords = anim.getCurrentTexCoords();
			vec2 top_left = coords.first;
			vec2 bottom_right = coords.second;

			TexturedVertex vertices[4] = {
				{{-0.5f, +0.5f, 0.f}, {top_left.x, bottom_right.y}},     // Top-left
				{{+0.5f, +0.5f, 0.f}, {bottom_right.x, bottom_right.y}}, // Top-right
				{{+0.5f, -0.5f, 0.f}, {bottom_right.x, top_left.y}},     // Bottom-right
				{{-0.5f, -0.5f, 0.f}, {top_left.x, top_left.y}}         // Bottom-left
			};

			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
		}

		else if (registry.robotAnimations.has(entity) && render_request.used_texture == TEXTURE_ASSET_ID::CROCKBOT_FULLSHEET || render_request.used_texture == TEXTURE_ASSET_ID::COMPANION_CROCKBOT_FULLSHEET) {
				// Player with animation
				const auto& anim = registry.robotAnimations.get(entity);
				std::pair<vec2, vec2> coords = anim.getCurrentTexCoords();
				vec2 top_left = coords.first;
				vec2 bottom_right = coords.second;


				TexturedVertex vertices[4] = {
				{{-0.5f, +0.5f, 0.f}, {top_left.x, bottom_right.y}},
				{{+0.5f, +0.5f, 0.f}, {bottom_right.x, bottom_right.y}},
				{{+0.5f, -0.5f, 0.f}, {bottom_right.x, top_left.y}},
				{{-0.5f, -0.5f, 0.f}, {top_left.x, top_left.y}}
				};

				glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
			//	drawRobotHealthBar(entity, projection);
			
		}
		else if (registry.iceRobotAnimations.has(entity) && render_request.used_texture == TEXTURE_ASSET_ID::ICE_ROBOT_FULLSHEET  || render_request.used_texture == TEXTURE_ASSET_ID::COMPANION_ICE_ROBOT_FULLSHEET) {
			// Player with animation
			const auto& anim = registry.iceRobotAnimations.get(entity);
			std::pair<vec2, vec2> coords = anim.getCurrentTexCoords();
			vec2 top_left = coords.first;
			vec2 bottom_right = coords.second;


			TexturedVertex vertices[4] = {
			{{-0.5f, +0.5f, 0.f}, {top_left.x, bottom_right.y}},
			{{+0.5f, +0.5f, 0.f}, {bottom_right.x, bottom_right.y}},
			{{+0.5f, -0.5f, 0.f}, {bottom_right.x, top_left.y}},
			{{-0.5f, -0.5f, 0.f}, {top_left.x, top_left.y}}
			};

			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
			//	drawRobotHealthBar(entity, projection);

		}
		else if (registry.bossRobotAnimations.has(entity) && render_request.used_texture == TEXTURE_ASSET_ID::BOSS_FULLSHEET) {
				// Player with animation
				const auto& anim = registry.bossRobotAnimations.get(entity);
				std::pair<vec2, vec2> coords = anim.getCurrentTexCoords();
				vec2 top_left = coords.first;
				vec2 bottom_right = coords.second;


				TexturedVertex vertices[4] = {
				{{-0.5f, +0.5f, 0.f}, {top_left.x, bottom_right.y}},
				{{+0.5f, +0.5f, 0.f}, {bottom_right.x, bottom_right.y}},
				{{+0.5f, -0.5f, 0.f}, {bottom_right.x, top_left.y}},
				{{-0.5f, -0.5f, 0.f}, {top_left.x, top_left.y}}
				};

				glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
		} 
		else if (registry.spiderRobotAnimations.has(entity) && render_request.used_texture == TEXTURE_ASSET_ID::SPIDERROBOT_FULLSHEET) {
			// Player with animation
			const auto& anim = registry.spiderRobotAnimations.get(entity);
			std::pair<vec2, vec2> coords = anim.getCurrentTexCoords();
			vec2 top_left = coords.first;
			vec2 bottom_right = coords.second;


			TexturedVertex vertices[4] = {
			{{-0.5f, +0.5f, 0.f}, {top_left.x, bottom_right.y}},
			{{+0.5f, +0.5f, 0.f}, {bottom_right.x, bottom_right.y}},
			{{+0.5f, -0.5f, 0.f}, {bottom_right.x, top_left.y}},
			{{-0.5f, -0.5f, 0.f}, {top_left.x, top_left.y}}
			};

			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
		}
		else {
			TexturedVertex vertices[4] = {
				{{-0.5f, +0.5f, 0.f}, {0.f, 1.f}},  
				{{+0.5f, +0.5f, 0.f}, {1.f, 1.f}}, 
				{{+0.5f, -0.5f, 0.f}, {1.f, 0.f}},
				{{-0.5f, -0.5f, 0.f}, {0.f, 0.f}} 
			};
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
		}
		
		gl_has_errors();

	}
	std::cerr << "debug6" << std::endl;

	// Set up vertex attributes
	GLint in_position_loc = glGetAttribLocation(program, "in_position");
	GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
	gl_has_errors();
	assert(in_texcoord_loc >= 0);
	std::cerr << "debug7" << std::endl;

	glEnableVertexAttribArray(in_position_loc);
	std::cerr << "debug at below line" << std::endl;
	gl_has_errors();
	std::cerr << "debug8" << std::endl;

	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
	gl_has_errors();

	glEnableVertexAttribArray(in_texcoord_loc);
	glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)sizeof(vec3));
	gl_has_errors();

	// Activate the texture
	glActiveTexture(GL_TEXTURE0);
	GLuint texture_id = texture_gl_handles[(GLuint)render_request.used_texture];
	glBindTexture(GL_TEXTURE_2D, texture_id);

	// Set texture parameters (optional for tiles)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl_has_errors();

	// Set uniform values for the shader
	GLint color_uloc = glGetUniformLocation(program, "fcolor");
	const vec3 color = registry.colors.has(entity) ? registry.colors.get(entity) : vec3(1);
	glUniform3fv(color_uloc, 1, (float*)&color);
	gl_has_errors();

	// Get the number of indices
	GLint size = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	gl_has_errors();

	GLsizei num_indices = size / sizeof(uint16_t);

	// Set transformation and projection uniforms
	GLuint transform_loc = glGetUniformLocation(program, "transform");
	glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&transform.mat);
	GLuint projection_loc = glGetUniformLocation(program, "projection");
	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection);
	gl_has_errors();

	// Draw the entity (tiles or otherwise)
	glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
	gl_has_errors();
}
void RenderSystem::drawToScreen()
{
	// Setting shaders
	glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::SCREEN]);
	gl_has_errors();

	// Clearing backbuffer
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, w, h);
	glDepthRange(0, 10);
	glClearColor(1.f, 0, 0, 1.0);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	gl_has_errors();

	// Enabling alpha channel for textures
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	// Bind geometry
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[(GLuint)GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffers[(GLuint)GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE]);
	gl_has_errors();

	// Get shader program
	const GLuint screen_program = effects[(GLuint)EFFECT_ASSET_ID::SCREEN];

	// Set uniforms
	GLuint fade_in_uloc = glGetUniformLocation(screen_program, "fade_in_factor");
	GLuint darken_uloc = glGetUniformLocation(screen_program, "darken_screen_factor");
	GLuint nighttime_uloc = glGetUniformLocation(screen_program, "nighttime_factor");

	ScreenState& screen = registry.screenStates.get(screen_state_entity);
	glUniform1f(fade_in_uloc, screen.fade_in_factor);
	glUniform1f(darken_uloc, screen.darken_screen_factor);
	glUniform1f(nighttime_uloc, screen.nighttime_factor);

	if (screen.nighttime_factor > 0.0f && registry.players.has(player)) {
		Motion& motion = registry.motions.get(player);
		vec2 player_world_position = motion.position;
		// if camera position is not moving  on the y axis
		float ndc_x;
		float ndc_y;
		
		if (camera_position.y == 0) {
			ndc_x = (player_world_position.x - camera_position.x) / window_width_px;
			ndc_y = 0.5f + -((player_world_position.y / window_height_px) - 0.5f);
		
		}
	
		else if (camera_position.y == 1840.0f) {
			int map_height_px = map_height * 64;
			ndc_x = (player_world_position.x - camera_position.x) / window_width_px;
		
			ndc_y = 2.465f - ((player_world_position.y ) / window_height_px) * 0.64f;

		}
		else {
			ndc_x = (player_world_position.x - camera_position.x) / window_width_px;
			ndc_y = (player_world_position.y - camera_position.y) / window_height_px;
		}
	//	printf("Camera Position: (%.2f, %.2f)\n", camera_position.x, camera_position.y);
		GLuint spotlight_center_uloc = glGetUniformLocation(screen_program, "spotlight_center");
		GLuint spotlight_radius_uloc = glGetUniformLocation(screen_program, "spotlight_radius");

		vec2 spotlight_center = vec2(ndc_x, ndc_y);
		float spotlight_radius = 0.25f; 

		glUniform2fv(spotlight_center_uloc, 1, glm::value_ptr(spotlight_center));
		glUniform1f(spotlight_radius_uloc, spotlight_radius);
	}
	//vec2 door_position = vec2(64 * 21, 64 * 7); // Door's position in world coordinates
	//float glow_radius = 0.2f;
	//float glow_intensity = 0.5f;

	//// Calculate the door position in texture coordinates relative to the camera
	//float glow_texcoord_x = (door_position.x - camera_position.x) / window_width_px;
	//float glow_texcoord_y = (door_position.y - camera_position.y) / window_height_px;

	//// Pass uniforms to the shader
	//GLuint glow_center_uloc = glGetUniformLocation(screen_program, "glow_center");
	//GLuint glow_radius_uloc = glGetUniformLocation(screen_program, "glow_radius");
	//GLuint glow_intensity_uloc = glGetUniformLocation(screen_program, "glow_intensity");

	//// Pass the current time to the shader
	//float current_time = static_cast<float>(glfwGetTime());
	//GLuint time_uloc = glGetUniformLocation(screen_program, "time");

	//glUniform1f(time_uloc, current_time);
	//glUniform2f(glow_center_uloc, glow_texcoord_x, glow_texcoord_y); // Glow center in texture coordinates
	//glUniform1f(glow_radius_uloc, glow_radius);
	//glUniform1f(glow_intensity_uloc, glow_intensity);

	//gl_has_errors();


	// Set vertex position and texture coordinates
	GLint in_position_loc = glGetAttribLocation(screen_program, "in_position");
	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
	gl_has_errors();

	// Bind the texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, off_screen_render_buffer_color);
	gl_has_errors();

	// Draw elements
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, nullptr);
	gl_has_errors();
}

void RenderSystem::drawSpaceshipTexture(Entity entity, const mat3& projection)
{
	assert(registry.motions.has(entity));
	Motion& motion = registry.motions.get(entity);
	assert(registry.renderRequests.has(entity));
	const RenderRequest& render_request = registry.renderRequests.get(entity);

	assert(render_request.used_texture == TEXTURE_ASSET_ID::SPACESHIP);

	GLuint program = effects[(GLuint)EFFECT_ASSET_ID::TEXTURED];
	glUseProgram(program);
	gl_has_errors();

	GLuint texture_id = texture_gl_handles[(GLuint)TEXTURE_ASSET_ID::SPACESHIP];
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	gl_has_errors();

	Transform transform;
	vec2 render_position = motion.position - camera_position;
	render_position.y += 5;
	render_position.x += -10;
	transform.translate(render_position);
	transform.rotate(motion.angle);
	transform.scale(motion.scale*1.12f);

	GLint transform_loc = glGetUniformLocation(program, "transform");
	GLint projection_loc = glGetUniformLocation(program, "projection");
	GLint texture_loc = glGetUniformLocation(program, "tex");

	glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&transform.mat);
	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection);
	glUniform1i(texture_loc, 0);
	gl_has_errors();

	TexturedVertex vertices[4] = {
		{{-0.5f, +0.5f, 0.f}, {0.f, 0.f}},
		{{+0.5f, +0.5f, 0.f}, {1.f, 0.f}},
		{{+0.5f, -0.5f, 0.f}, {1.f, 1.f}},
		{{-0.5f, -0.5f, 0.f}, {0.f, 1.f}}
	};

	static const uint16_t indices[] = { 0, 1, 2, 2, 3, 0 };

	GLuint vbo, ibo;
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ibo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	GLint in_position_loc = glGetAttribLocation(program, "in_position");
	GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
	gl_has_errors();

	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
	glEnableVertexAttribArray(in_texcoord_loc);
	glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)sizeof(vec3));
	gl_has_errors();

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
	gl_has_errors();

	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ibo);
}




// Render our game world
// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-14-render-to-texture/
void RenderSystem::draw()
{
	if (show_start_screen) {
		int w, h;
		glfwGetFramebufferSize(window, &w, &h);


		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		gl_has_errors();

		glClearColor(0.f, 0.f, 0.f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		 
		glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::SCREEN]);
		gl_has_errors();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture_gl_handles[(GLuint)TEXTURE_ASSET_ID::START_SCREEN]);
		gl_has_errors();

		glBindVertexArray(startscreen_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
		gl_has_errors();

		renderStartScreen();

		glfwSwapBuffers(window);
		return;
	}

	if (playing_cutscene) {
		renderCutscene();
		glfwSwapBuffers(window);
		return;
	}


	// Getting size of window
	int w, h;
	glfwGetFramebufferSize(window, &w, &h); // Note, this will be 2x the resolution given to glfwCreateWindow on retina displays

	// First render to the custom framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	gl_has_errors();
	// Clearing backbuffer
	glViewport(0, 0, w, h);
	glDepthRange(0.00001, 10);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(10.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST); // native OpenGL does not work with a depth buffer
							  // and alpha blending, one would have to sort
							  // sprites back to front
	gl_has_errors();
	mat3 projection_2D = createProjectionMatrix();
	mat3 ui_projection = createOrthographicProjection(0, window_width_px, 0, window_height_px);

	
	// Draw all textured meshes that have a position and size component
	float camera_left = camera_position.x;
	float camera_right = camera_position.x + window_width_px;
	float camera_top = camera_position.y;
	float camera_bottom = camera_position.y + window_height_px;


	/*for (Entity entity : registry.tiles.entities) {
		if (!registry.motions.has(entity)) continue;
		drawTexturedMesh(entity, projection_2D);
	}*/
	for (Entity entity : registry.tiles.entities) {
		if (!registry.motions.has(entity)) continue;

		// Get tile position and size
		Motion& motion = registry.motions.get(entity);
		vec2 tile_position = motion.position;
		vec2 tile_size = abs(motion.scale);

		// Calculate tile boundaries
		float tile_left = tile_position.x - tile_size.x / 2;
		float tile_right = tile_position.x + tile_size.x / 2;
		float tile_top = tile_position.y - tile_size.y / 2;
		float tile_bottom = tile_position.y + tile_size.y / 2;

		// Check if the tile is within the camera's frame
		if (tile_right < camera_left || tile_left > camera_right ||
			tile_bottom < camera_top || tile_top > camera_bottom) {
			continue; // Skip rendering tiles outside the camera view
		}

		std::cerr << "debug1" << std::endl;

		drawTexturedMesh(entity, projection_2D);
	}

	// todo - change this

	for (Entity entity : registry.boids.entities) {
		if (!registry.motions.has(entity)) continue;

		Motion& motion = registry.motions.get(entity);
		vec2 boid_position = motion.position;
		vec2 boid_size = abs(motion.scale);

		float boid_left = boid_position.x - boid_size.x / 2;
		float boid_right = boid_position.x + boid_size.x / 2;
		float boid_top = boid_position.y - boid_size.y / 2;
		float boid_bottom = boid_position.y + boid_size.y / 2;

		if (boid_right >= camera_left && boid_left <= camera_right &&
			boid_bottom >= camera_top && boid_top <= camera_bottom) {
			drawTexturedMesh(entity, projection_2D);
		}
	}

	// Draw robots within the camera frame
	for (Entity entity : registry.robots.entities) {
		if (!registry.motions.has(entity)) continue;

		// Get robot position and size
		Motion& motion = registry.motions.get(entity);
		vec2 robot_position = motion.position;
		vec2 robot_size = abs(motion.scale);

		// Calculate robot boundaries
		float robot_left = robot_position.x - robot_size.x / 2;
		float robot_right = robot_position.x + robot_size.x / 2;
		float robot_top = robot_position.y - robot_size.y / 2;
		float robot_bottom = robot_position.y + robot_size.y / 2;

		// Check if the robot is within the camera's frame
		if (robot_right >= camera_left && robot_left <= camera_right &&
			robot_bottom >= camera_top && robot_top <= camera_bottom) {
			// check if entity contains registry.iceRobotAnimations.get(entity); {if so print hello world}
			drawRobotHealthBar(entity, projection_2D);
			drawTexturedMesh(entity, projection_2D);
		}
	}


	for (Entity entity : registry.bossRobots.entities) {
		if (!registry.motions.has(entity)) continue;

		// Get robot position and size
		Motion& motion = registry.motions.get(entity);
		vec2 robot_position = motion.position;
		vec2 robot_size = abs(motion.scale);

		// Calculate robot boundaries
		float robot_left = robot_position.x - robot_size.x / 2;
		float robot_right = robot_position.x + robot_size.x / 2;
		float robot_top = robot_position.y - robot_size.y / 2;
		float robot_bottom = robot_position.y + robot_size.y / 2;

		// Check if the robot is within the camera's frame
		if (robot_right >= camera_left && robot_left <= camera_right &&
			robot_bottom >= camera_top && robot_top <= camera_bottom) {
			drawTexturedMesh(entity, projection_2D);
			drawBossRobotHealthBar(entity, projection_2D);
		}
	}

	for (Entity entity : registry.particles.entities) {
		drawTexturedMesh(entity, projection_2D);
	}
	for (Entity entity : registry.spiderRobots.entities) {
		if (!registry.motions.has(entity)) continue;
		drawTexturedMesh(entity, projection_2D);
	}

	if (registry.players.has(player)) {
		drawTexturedMesh(player, projection_2D);
		
	}

	for (Entity entity : registry.doors.entities) {
		if (!registry.motions.has(entity)) continue;
		drawTexturedMesh(entity, projection_2D);
	}

	for (Entity entity : registry.potions.entities) {
		if (!registry.motions.has(entity)) continue;
		drawTexturedMesh(entity, projection_2D);
	}

	for (Entity entity : registry.keys.entities) {
		if (!registry.motions.has(entity)) continue;
		drawTexturedMesh(entity, projection_2D);
	}
	for (Entity entity : registry.armorplates.entities) {
		if (!registry.motions.has(entity)) continue;
		drawTexturedMesh(entity, projection_2D);
	}

	for (Entity entity : registry.spaceships.entities) {
		if (!registry.motions.has(entity)) continue;
		Motion& motion = registry.motions.get(entity);
		drawTexturedMesh(entity, projection_2D);
		drawSpaceshipTexture(entity, projection_2D);
	}


	for (Entity entity : registry.projectile.entities) {
		if (!registry.motions.has(entity)) continue;

		// Get projectile position and size
		Motion& motion = registry.motions.get(entity);
		vec2 projectile_position = motion.position;
		vec2 projectile_size = abs(motion.scale);

		// Calculate projectile boundaries
		float projectile_left = projectile_position.x - projectile_size.x / 2;
		float projectile_right = projectile_position.x + projectile_size.x / 2;
		float projectile_top = projectile_position.y - projectile_size.y / 2;
		float projectile_bottom = projectile_position.y + projectile_size.y / 2;

		// Check if the projectile is within the camera's frame
		if (projectile_right >= camera_left && projectile_left <= camera_right &&
			projectile_bottom >= camera_top && projectile_top <= camera_bottom) {
			drawTexturedMesh(entity, projection_2D);
		}
	}

	for (Entity entity : registry.bossProjectile.entities) {
		if (!registry.motions.has(entity)) continue;

		// Get projectile position and size
		Motion& motion = registry.motions.get(entity);
		vec2 projectile_position = motion.position;
		vec2 projectile_size = abs(motion.scale);

		// Calculate projectile boundaries
		float projectile_left = projectile_position.x - projectile_size.x / 2;
		float projectile_right = projectile_position.x + projectile_size.x / 2;
		float projectile_top = projectile_position.y - projectile_size.y / 2;
		float projectile_bottom = projectile_position.y + projectile_size.y / 2;

		// Check if the projectile is within the camera's frame
		if (projectile_right >= camera_left && projectile_left <= camera_right &&
			projectile_bottom >= camera_top && projectile_top <= camera_bottom) {
			drawTexturedMesh(entity, projection_2D);
		}
	}

	drawToScreen();
	drawHUD(player, ui_projection);
	Inventory& inventory = registry.players.get(player).inventory;
	if (inventory.isOpen) {
		drawInventoryUI();
	}
	for (auto entity : registry.robots.entities) {
		Robot& robot = registry.robots.get(entity);
		if (robot.showCaptureUI) {
			currentRobotEntity = entity;
			renderCaptureUI(robot, entity);
			show_capture_ui = true;
		}
	}

	if (key_spawned) {
		glm::vec3 font_color = glm::vec3(1.0f, 1.0f, 1.0f); // White color
		glm::mat4 font_trans = glm::mat4(1.0f); // Identity matrix
		renderText("Key Spawned!", window_width_px - 200.0f, 20.0f, 0.5f, font_color, font_trans);
	}
	// Truely render to the screen

	helpOverlay.render();

	// Update and display FPS
	updateFPS();

	// Draw FPS counter on the screen
	if (show_fps) {
		drawFPSCounter(createOrthographicProjection(0, window_width_px, 0, window_height_px));
	}
	if (game_paused) {
		drawPausedUI(ui_projection);
	}

	// flicker-free display with a double buffer
	glfwSwapBuffers(window);
	gl_has_errors();
}

void RenderSystem::drawPausedUI(const mat3& projection) {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	vec2 bar_position = vec2(40.f, window_height_px - 60.f);
	vec2 bar_size = vec2(200.f, 20.f);

	vec2 avatar_size = vec2(1280.f, 720.f);
	TexturedVertex avatar_vertices[4] = {
	{ vec3(0.f, 0.f, 0.f), vec2(0.f, 0.f) },                
	{ vec3(window_width_px, 0.f, 0.f), vec2(1.f, 0.f) },    
	{ vec3(window_width_px, window_height_px, 0.f), vec2(1.f, 1.f) }, 
	{ vec3(0.f, window_height_px, 0.f), vec2(0.f, 1.f) }
	};
	glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED]);
	gl_has_errors();
	glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(avatar_vertices), avatar_vertices, GL_DYNAMIC_DRAW);
	gl_has_errors();

	GLint in_position_loc = glGetAttribLocation(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED], "in_position");
	GLint in_texcoord_loc = glGetAttribLocation(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED], "in_texcoord");

	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
	glEnableVertexAttribArray(in_texcoord_loc);
	glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)sizeof(vec3));
	gl_has_errors();

	glActiveTexture(GL_TEXTURE0);
	GLuint avatar_texture_id = texture_gl_handles[(GLuint)TEXTURE_ASSET_ID::PAUSED_UI];
	glBindTexture(GL_TEXTURE_2D, avatar_texture_id);
	gl_has_errors();
	GLuint transform_loc = glGetUniformLocation(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED], "transform");
	mat3 identity_transform = mat3(1.0f); 
	glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&identity_transform);
	GLuint projection_loc = glGetUniformLocation(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED], "projection");
	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection);
	gl_has_errors();

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	gl_has_errors();
	struct MenuOption {
		const char* text;
		float x, y;
		glm::vec3 default_color;
		glm::vec3 hover_color;
	};

	MenuOption menu_options[] = {
		{"Resume", 420.0f, 470.0f, glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.937f, 0.745f, 0.035f)},
		{"Help [H]", 420.0f, 400.0f, glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.937f, 0.745f, 0.035f)},
		{"Save and Quit", 420.0f, 330.0f, glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.937f, 0.745f, 0.035f)},
		{"Restart Game", 420.0f, 260.0f, glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.937f, 0.745f, 0.035f)}
	};

	double mouse_x, mouse_y;
	glfwGetCursorPos(window, &mouse_x, &mouse_y);
	mouse_y = window_height_px - mouse_y; 

	hovered_menu_index = -1; 

	for (int i = 0; i < 4; i++) {
		MenuOption& option = menu_options[i];

		float text_width = 150.0f * 1.3f;
		float text_height = 30.0f * 1.3f; 
		float text_left = option.x;
		float text_right = text_left + text_width;
		float text_bottom = option.y;
		float text_top = text_bottom + text_height;

		bool is_hovered = (mouse_x >= text_left && mouse_x <= text_right &&
			mouse_y >= text_bottom && mouse_y <= text_top);

		glm::vec3 font_color = is_hovered ? option.hover_color : option.default_color;

		glm::mat4 font_trans = glm::mat4(1.0f); 
		renderText(option.text, option.x, option.y, 1.3f, font_color, font_trans);

		if (is_hovered) {
			hovered_menu_index = i;
		}
	}
}
mat3 RenderSystem::createProjectionMatrix()
{
	// Fake projection matrix, scales with respect to window coordinates
	float left = 0.f;
	float top = 0.f;

	gl_has_errors();
	float right = (float) window_width_px;
	float bottom = (float) window_height_px;

	float sx = 2.f / (right - left);
	float sy = 2.f / (top - bottom);
	float tx = -(right + left) / (right - left);
	float ty = -(top + bottom) / (top - bottom);
	return {{sx, 0.f, 0.f}, {0.f, sy, 0.f}, {tx, ty, 1.f}};
}
void RenderSystem::initHealthBarVBO() {
	if (!healthbar_vbo_initialized) {
		glGenVertexArrays(1, &healthbar_vao);
		glGenBuffers(1, &healthbar_vbo);

		glBindVertexArray(healthbar_vao);

		glBindBuffer(GL_ARRAY_BUFFER, healthbar_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);  // Reserve space for vertices

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);

		healthbar_vbo_initialized = true;
	}
}

void RenderSystem::drawHUD(Entity player, const mat3& projection)
{
	if (!registry.players.has(player))
		return;
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// Get player health values
	Player& player_data = registry.players.get(player);
	Inventory& player_inventory = player_data.inventory;
	const auto& items = player_inventory.getItems();
	Entity radiation_entity = *registry.radiations.entities.begin();
	Radiation& radiation_data = registry.radiations.get(radiation_entity);

	float radiation_percentage = radiation_data.intensity / 1.0f;

	vec2 radiation_bar_position = vec2(window_width_px - 240.f, 20.f);
	vec2 radiation_bar_size = vec2(170.f, 20.f);
	/*glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	gl_has_errors();*/

	// Radiation icon position and size


	TexturedVertex radiation_full_bar_vertices[4] = {
		{ vec3(radiation_bar_position.x, radiation_bar_position.y, 0.f), vec2(0.f, 1.f) },  // Top-left
		{ vec3(radiation_bar_position.x + radiation_bar_size.x, radiation_bar_position.y, 0.f), vec2(1.f, 1.f) }, // Top-right
		{ vec3(radiation_bar_position.x + radiation_bar_size.x, radiation_bar_position.y + radiation_bar_size.y, 0.f), vec2(1.f, 0.f) }, // Bottom-right
		{ vec3(radiation_bar_position.x, radiation_bar_position.y + radiation_bar_size.y, 0.f), vec2(0.f, 0.f) } // Bottom-left
	};

	TexturedVertex radiation_current_bar_vertices[4] = {
		{ vec3(radiation_bar_position.x, radiation_bar_position.y, 0.f), vec2(0.f, 1.f) },  // Top-left
		{ vec3(radiation_bar_position.x + radiation_bar_size.x * radiation_percentage, radiation_bar_position.y, 0.f), vec2(1.f, 1.f) }, // Top-right (scaled by radiation)
		{ vec3(radiation_bar_position.x + radiation_bar_size.x * radiation_percentage, radiation_bar_position.y + radiation_bar_size.y, 0.f), vec2(1.f, 0.f) }, // Bottom-right
		{ vec3(radiation_bar_position.x, radiation_bar_position.y + radiation_bar_size.y, 0.f), vec2(0.f, 0.f) } // Bottom-left
	};
	// Health percentage (between 0 and 1)
	float health_percentage = player_data.current_health / player_data.max_health;

	// Health bar position and size
	vec2 bar_position = vec2(40.f, window_height_px - 60.f); 
	vec2 bar_size = vec2(200.f, 20.f);                    

	// Full bar (gray background)
	TexturedVertex full_bar_vertices[4] = {
		{ vec3(bar_position.x, bar_position.y, 0.f), vec2(0.f, 1.f) },  // Top-left
		{ vec3(bar_position.x + bar_size.x, bar_position.y, 0.f), vec2(1.f, 1.f) }, // Top-right
		{ vec3(bar_position.x + bar_size.x, bar_position.y + bar_size.y, 0.f), vec2(1.f, 0.f) }, // Bottom-right
		{ vec3(bar_position.x, bar_position.y + bar_size.y, 0.f), vec2(0.f, 0.f) } // Bottom-left
	};

	// Current health bar (green-to-red portion)
	TexturedVertex current_bar_vertices[4] = {
		{ vec3(bar_position.x, bar_position.y, 0.f), vec2(0.f, 1.f) },  // Top-left
		{ vec3(bar_position.x + bar_size.x * health_percentage, bar_position.y, 0.f), vec2(1.f, 1.f) }, // Top-right (scaled by health)
		{ vec3(bar_position.x + bar_size.x * health_percentage, bar_position.y + bar_size.y, 0.f), vec2(1.f, 0.f) }, // Bottom-right
		{ vec3(bar_position.x, bar_position.y + bar_size.y, 0.f), vec2(0.f, 0.f) } // Bottom-left
	};

	vec2 stamina_bar_position = vec2(bar_position.x, bar_position.y + bar_size.y);
	vec2 stamina_bar_size = vec2(bar_size.x, bar_size.y / 2.f);

	float stamina_percentage = player_data.current_stamina / player_data.max_stamina;

	TexturedVertex stamina_bar_vertices[4] = {
	{ vec3(stamina_bar_position.x, stamina_bar_position.y, 0.f), vec2(0.f, 1.f) },
	{ vec3(stamina_bar_position.x + stamina_bar_size.x * stamina_percentage, stamina_bar_position.y, 0.f), vec2(1.f, 1.f) },
	{ vec3(stamina_bar_position.x + stamina_bar_size.x * stamina_percentage, stamina_bar_position.y + stamina_bar_size.y, 0.f), vec2(1.f, 0.f) },
	{ vec3(stamina_bar_position.x, stamina_bar_position.y + stamina_bar_size.y, 0.f), vec2(0.f, 0.f) }
	};
	TexturedVertex stamina_full_bar_vertices[4] = {
	{ vec3(stamina_bar_position.x, stamina_bar_position.y, 0.f), vec2(0.f, 1.f) },  // Top-left
	{ vec3(stamina_bar_position.x + stamina_bar_size.x, stamina_bar_position.y, 0.f), vec2(1.f, 1.f) }, // Top-right
	{ vec3(stamina_bar_position.x + stamina_bar_size.x, stamina_bar_position.y + stamina_bar_size.y, 0.f), vec2(1.f, 0.f) }, // Bottom-right
	{ vec3(stamina_bar_position.x, stamina_bar_position.y + stamina_bar_size.y, 0.f), vec2(0.f, 0.f) } // Bottom-left
	};
	// Draw the avatar above the health bar
	vec2 avatar_size = vec2(128.f, 128.f); 
	vec2 avatar_position = vec2(bar_position.x + (bar_size.x / 2) - (avatar_size.x / 2), bar_position.y - 125.f); // Center above health bar

	// Create vertices for the avatar (textured quad)
	TexturedVertex avatar_vertices[4] = {
		{ vec3(avatar_position.x, avatar_position.y, 0.f), vec2(0.f, 0.f) },  // Top-left (flipped)
		{ vec3(avatar_position.x + avatar_size.x, avatar_position.y, 0.f), vec2(1.f, 0.f) }, // Top-right (flipped)
		{ vec3(avatar_position.x + avatar_size.x, avatar_position.y + avatar_size.y, 0.f), vec2(1.f, 1.f) }, // Bottom-right
		{ vec3(avatar_position.x, avatar_position.y + avatar_size.y, 0.f), vec2(0.f, 1.f) } // Bottom-left
	};

	glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED]);
	gl_has_errors();

	// Bind the VBO for avatar rendering 
	glBindBuffer(GL_ARRAY_BUFFER, healthbar_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(avatar_vertices), avatar_vertices, GL_DYNAMIC_DRAW);
	gl_has_errors();

	GLint in_position_loc = glGetAttribLocation(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED], "in_position");
	GLint in_texcoord_loc = glGetAttribLocation(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED], "in_texcoord");

	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
	glEnableVertexAttribArray(in_texcoord_loc);
	glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)sizeof(vec3));
	gl_has_errors();

	glActiveTexture(GL_TEXTURE0);
	GLuint avatar_texture_id = texture_gl_handles[(GLuint)TEXTURE_ASSET_ID::AVATAR]; 
	glBindTexture(GL_TEXTURE_2D, avatar_texture_id);
	gl_has_errors();

	// Set transformation and projection uniforms for the avatar
	GLuint transform_loc = glGetUniformLocation(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED], "transform");
	mat3 identity_transform = mat3(1.0f); // No transform needed for UI
	glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&identity_transform);
	GLuint projection_loc = glGetUniformLocation(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED], "projection");
	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection);
	gl_has_errors();

	// Draw the avatar
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	gl_has_errors();


	vec2 armor_icon_size = avatar_size * 0.2f;
	vec2 armor_icon_position = vec2(
		40.f,
		avatar_position.y + 10.0f
	);

	TexturedVertex armor_icon_vertices[4] = {
		{ vec3(armor_icon_position.x, armor_icon_position.y, 0.f), vec2(0.f, 0.f) },
		{ vec3(armor_icon_position.x + armor_icon_size.x, armor_icon_position.y, 0.f), vec2(1.f, 0.f) },
		{ vec3(armor_icon_position.x + armor_icon_size.x, armor_icon_position.y + armor_icon_size.y, 0.f), vec2(1.f, 1.f) },
		{ vec3(armor_icon_position.x, armor_icon_position.y + armor_icon_size.y, 0.f), vec2(0.f, 1.f) }
	};

	glBufferData(GL_ARRAY_BUFFER, sizeof(armor_icon_vertices), armor_icon_vertices, GL_DYNAMIC_DRAW);
	gl_has_errors();
	GLuint armor_icon_texture_id = texture_gl_handles[(GLuint)TEXTURE_ASSET_ID::ARMOR_ICON];
	glBindTexture(GL_TEXTURE_2D, armor_icon_texture_id);
	gl_has_errors();
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	gl_has_errors();

	vec2 weapon_icon_size = avatar_size * 0.2f;
	vec2 weapon_icon_position = vec2(
		40.f,
		avatar_position.y + 45.0f
	);

	TexturedVertex weapon_icon_vertices[4] = {
		{ vec3(weapon_icon_position.x, weapon_icon_position.y, 0.f), vec2(0.f, 0.f) },
		{ vec3(weapon_icon_position.x + weapon_icon_size.x, weapon_icon_position.y, 0.f), vec2(1.f, 0.f) },
		{ vec3(weapon_icon_position.x + weapon_icon_size.x, weapon_icon_position.y + weapon_icon_size.y, 0.f), vec2(1.f, 1.f) },
		{ vec3(weapon_icon_position.x, weapon_icon_position.y + weapon_icon_size.y, 0.f), vec2(0.f, 1.f) }
	};

	glBufferData(GL_ARRAY_BUFFER, sizeof(weapon_icon_vertices), weapon_icon_vertices, GL_DYNAMIC_DRAW);
	gl_has_errors();
	GLuint weapon_icon_texture_id = texture_gl_handles[(GLuint)TEXTURE_ASSET_ID::WEAPON_ICON];
	glBindTexture(GL_TEXTURE_2D, weapon_icon_texture_id);
	gl_has_errors();
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	gl_has_errors();
	// radiation
	vec2 radiation_icon_size = vec2(30.f, 30.f);
	vec2 radiation_icon_position = vec2(radiation_bar_position.x - radiation_icon_size.x - 10.f, radiation_bar_position.y - 3.0f);
	GLuint radiation_icon_texture_id = texture_gl_handles[(GLuint)TEXTURE_ASSET_ID::RADIATION_ICON];

	TexturedVertex radiation_icon_vertices[4] = {
		{ vec3(radiation_icon_position.x, radiation_icon_position.y, 0.f), vec2(0.f, 1.f) },
		{ vec3(radiation_icon_position.x + radiation_icon_size.x, radiation_icon_position.y, 0.f), vec2(1.f, 1.f) },
		{ vec3(radiation_icon_position.x + radiation_icon_size.x, radiation_icon_position.y + radiation_icon_size.y, 0.f), vec2(1.f, 0.f) },
		{ vec3(radiation_icon_position.x, radiation_icon_position.y + radiation_icon_size.y, 0.f), vec2(0.f, 0.f) }
	};
	glBindTexture(GL_TEXTURE_2D, radiation_icon_texture_id);
	glBufferData(GL_ARRAY_BUFFER, sizeof(radiation_icon_vertices), radiation_icon_vertices, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	gl_has_errors();

	// weapon text rendering
	glm::vec3 font_color = glm::vec3(1.0f, 1.0f, 1.0f); // White color
	glm::mat4 font_trans = glm::mat4(1.0f); // Identity matrix
	std::string armor_text = std::to_string((int)player_data.armor_stat);
	renderText(armor_text, 70.0f, 160.0f, 0.35f, font_color, font_trans);
	std::string weapon_text = std::to_string((int)player_data.weapon_stat);
	renderText(weapon_text, 70.0f, 125.0f, 0.35f, font_color, font_trans);
	// Switch back to COLOURED shader for the health bar
	glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::COLOURED]);
	gl_has_errors();

	// Bind the VBO for the health bar
	glBindBuffer(GL_ARRAY_BUFFER, healthbar_vbo);

	// Draw the full (background) health bar
	glBufferData(GL_ARRAY_BUFFER, sizeof(full_bar_vertices), full_bar_vertices, GL_DYNAMIC_DRAW);
	gl_has_errors();

	// Set up vertex attributes for the health bar
	in_position_loc = glGetAttribLocation(effects[(GLuint)EFFECT_ASSET_ID::COLOURED], "in_position");
	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
	gl_has_errors();

	// Set the uniform color for the full bar (gray background)
	GLint color_uloc = glGetUniformLocation(effects[(GLuint)EFFECT_ASSET_ID::COLOURED], "fcolor");
	vec3 full_bar_color = vec3(0.7f, 0.7f, 0.7f); // Gray color
	glUniform3fv(color_uloc, 1, (float*)&full_bar_color);
	gl_has_errors();

	// Set transformation and projection matrices for the health bar
	transform_loc = glGetUniformLocation(effects[(GLuint)EFFECT_ASSET_ID::COLOURED], "transform");
	glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&identity_transform);
	projection_loc = glGetUniformLocation(effects[(GLuint)EFFECT_ASSET_ID::COLOURED], "projection");
	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection);
	gl_has_errors();

	// Draw the health bar
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	gl_has_errors();

	// Interpolate color between green (0, 1, 0) and red (1, 0, 0) based on health percentage
	vec3 health_color = glm::mix(vec3(1.0f, 0.0f, 0.0f), vec3(0.0627f, 0.8157f, 0.0f), health_percentage);
	// Draw the current health bar
	glBufferData(GL_ARRAY_BUFFER, sizeof(current_bar_vertices), current_bar_vertices, GL_DYNAMIC_DRAW);
	gl_has_errors();
	// Set the interpolated color for the current health bar
	glUniform3fv(color_uloc, 1, (float*)&health_color);
	gl_has_errors();
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	gl_has_errors();

	// STAMINA BAR
	glBufferData(GL_ARRAY_BUFFER, sizeof(stamina_full_bar_vertices), stamina_full_bar_vertices, GL_DYNAMIC_DRAW);
	gl_has_errors();
	glUniform3fv(color_uloc, 1, (float*)&full_bar_color);
	gl_has_errors();
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	gl_has_errors();
	glBufferData(GL_ARRAY_BUFFER, sizeof(stamina_bar_vertices), stamina_bar_vertices, GL_DYNAMIC_DRAW);
	vec3 stamina_color = vec3(0.1804f, 0.5137f, 0.8667f);
	glUniform3fv(color_uloc, 1, (float*)&stamina_color);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	// radiation bar

	// Draw the full radiation bar (background)
	glBufferData(GL_ARRAY_BUFFER, sizeof(radiation_full_bar_vertices), radiation_full_bar_vertices, GL_DYNAMIC_DRAW);
	gl_has_errors();
	glUniform3fv(color_uloc, 1, (float*)&full_bar_color);
	gl_has_errors();
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	gl_has_errors();

	glBufferData(GL_ARRAY_BUFFER, sizeof(radiation_current_bar_vertices), radiation_current_bar_vertices, GL_DYNAMIC_DRAW);
	gl_has_errors();

	vec3 radiation_color;
	float t = radiation_percentage;
	vec3 start_color = vec3(1.0f, 0.65f, 0.0f);
	vec3 mid_color = vec3(1.0f, 0.5f, 0.0f);
	vec3 end_color = vec3(235.0f / 255.0f, 4.0f / 255.0f, 0.0f);

	if (t < 0.5f) {
		radiation_color = glm::mix(start_color, mid_color, t * 2.0f);
	}
	else {
		radiation_color = glm::mix(mid_color, end_color, (t - 0.5f) * 2.0f);
	}


	glUniform3fv(color_uloc, 1, (float*)&radiation_color);
	gl_has_errors();
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	gl_has_errors();
	// Draw the "Health" label below the health bar
	float text_scale = 0.35f;

	float text_x = bar_position.x;
	float text_y = 45.0f;
	float health_text_x = bar_position.x - 27.0f;

	renderText("HP", health_text_x, text_y, text_scale, font_color, font_trans);
	std::string percentage_text = std::to_string((int)player_data.current_health) + "/" + std::to_string((int)player_data.max_health);
	float percentage_text_x = bar_position.x + 5.0f;
	renderText(percentage_text, percentage_text_x, text_y, text_scale, font_color, font_trans);
	renderText("STA", health_text_x, text_y - 14.0f, text_scale, font_color, font_trans);
	// Inventory Slots
	vec2 slot_size = vec2(190.f, 110.f);
	float total_slots_width = (3 * slot_size.x) / 1.5;  // 3 slots
	vec2 slot_position = vec2((window_width_px - total_slots_width) / 2, bar_position.y - slot_size.y + 10.f); 
	// Draw three inventory slots centered on the screen
	for (int i = 0; i < 3; ++i) {
		vec2 current_slot_position = slot_position + vec2(i * (slot_size.x) / 1.5, 0.f);

		// Draw Slot Background
		glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED]);
		gl_has_errors();

		TexturedVertex slot_vertices[4] = {
			{ vec3(current_slot_position.x, current_slot_position.y, 0.f), vec2(0.f, 1.f) },
			{ vec3(current_slot_position.x + slot_size.x, current_slot_position.y, 0.f), vec2(1.f, 1.f) },
			{ vec3(current_slot_position.x + slot_size.x, current_slot_position.y + slot_size.y, 0.f), vec2(1.f, 0.f) },
			{ vec3(current_slot_position.x, current_slot_position.y + slot_size.y, 0.f), vec2(0.f, 0.f) }
		};

		glBindBuffer(GL_ARRAY_BUFFER, healthbar_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(slot_vertices), slot_vertices, GL_DYNAMIC_DRAW);
		gl_has_errors();

		GLint in_position_loc = glGetAttribLocation(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED], "in_position");
		GLint in_texcoord_loc = glGetAttribLocation(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED], "in_texcoord");
		glEnableVertexAttribArray(in_position_loc);
		glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
		glEnableVertexAttribArray(in_texcoord_loc);
		glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)sizeof(vec3));
		gl_has_errors();

		GLuint slot_texture_id = (i == player_inventory.getSelectedSlot())
			? texture_gl_handles[(GLuint)TEXTURE_ASSET_ID::INVENTORY_SLOT_SELECTED]
			: texture_gl_handles[(GLuint)TEXTURE_ASSET_ID::INVENTORY_SLOT];

		glBindTexture(GL_TEXTURE_2D, slot_texture_id);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		gl_has_errors();

		if (i < player_inventory.slots.size()) {
			const auto& slot = player_inventory.slots[i];
			if (!slot.item.name.empty()) {
				TEXTURE_ASSET_ID item_texture_enum = getTextureIDFromItemName(slot.item.name);
				GLuint item_texture_id = texture_gl_handles[(GLuint)item_texture_enum];

				float scale_factor = std::min(slot_size.x / texture_dimensions[(GLuint)item_texture_enum].x,
					slot_size.y / texture_dimensions[(GLuint)item_texture_enum].y);
				vec2 item_size = vec2(texture_dimensions[(GLuint)item_texture_enum]) * scale_factor * 0.5f;
				vec2 item_position = current_slot_position + (slot_size - item_size) / 2.0f;

				TexturedVertex item_vertices[4] = {
					{ vec3(item_position.x, item_position.y, 0.f), vec2(0.f, 0.f) },
					{ vec3(item_position.x + item_size.x, item_position.y, 0.f), vec2(1.f, 0.f) },
					{ vec3(item_position.x + item_size.x, item_position.y + item_size.y, 0.f), vec2(1.f, 1.f) },
					{ vec3(item_position.x, item_position.y + item_size.y, 0.f), vec2(0.f, 1.f) }
				};

				glBufferData(GL_ARRAY_BUFFER, sizeof(item_vertices), item_vertices, GL_DYNAMIC_DRAW);
				glBindTexture(GL_TEXTURE_2D, item_texture_id);
				glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
				gl_has_errors();

				std::string count_text = std::to_string(slot.item.quantity);

				float text_scale = 0.5f;
				float count_x = current_slot_position.x + 117.f; 
				float count_y = current_slot_position.y - 425.f; 
				vec3 font_color = vec3(1.0f, 1.0f, 1.0f); 
				mat4 font_transform = mat4(1.0f); 

				glm::mat4 font_trans = glm::mat4(1.0f);
				renderText(count_text, count_x, count_y, text_scale, font_color, font_transform);
			}
		}
	}
	for (Entity entity : registry.notifications.entities) {
		const Notification& notification = registry.notifications.get(entity);

		float alpha = 1.0f - (notification.elapsed_time / notification.duration);
		vec3 color_with_alpha = notification.color * vec3(1.0f, 1.0f, 1.0f) * alpha;

		float textWidth = getTextWidth(notification.text, notification.scale); 
		float centeredX = notification.position.x - (textWidth / 2.0f);
		renderText(notification.text, centeredX, notification.position.y, notification.scale, color_with_alpha, mat4(1.0f));
	}
	if (tutorial_state != TutorialState::COMPLETED) {
		renderText("Press ENTER to skip tutorial", window_width_px - 350.0f, 10.0f, 0.5f, vec3(1.0f, 1.0f, 1.0f), mat4(1.0f));
	}
}

float RenderSystem::getTextWidth(const std::string& text, float scale) {
	float totalWidth = 0.0f;
	for (char c : text) {
		auto it = Characters.find(c);
		if (it != Characters.end()) {
			totalWidth += (it->second.advance >> 6) * scale; 
		}
		else {
			totalWidth += 10 * scale;
		}
	}
	return totalWidth;
}

TEXTURE_ASSET_ID RenderSystem::getTextureIDFromItemName(const std::string& itemName) {
	if (itemName == "Key") return TEXTURE_ASSET_ID::KEY;
	if (itemName == "ArmorPlate") return TEXTURE_ASSET_ID::ARMORPLATE;
	if (itemName == "HealthPotion") return TEXTURE_ASSET_ID::HEALTHPOTION;
	if (itemName == "CompanionRobot") return TEXTURE_ASSET_ID::COMPANION_CROCKBOT;
	if (itemName == "Energy Core") return TEXTURE_ASSET_ID::ENERGY_CORE;
	if (itemName == "Robot Parts") return TEXTURE_ASSET_ID::ROBOT_PART;
	if (itemName == "Teleporter") return TEXTURE_ASSET_ID::TELEPORTER;
	if (itemName == "IceRobot") return TEXTURE_ASSET_ID::ICE_ROBOT;
	return TEXTURE_ASSET_ID::TEXTURE_COUNT;// default (should replace with empty)
}



void RenderSystem::updateCameraPosition(vec2 player_position) {
	// Calculate map dimensions in pixels
	float map_width_px = 64 * map_width;
	float map_height_px = 64 * map_height;

	// Calculate half the window dimensions
	float half_window_width = window_width_px / 2;
	float half_window_height = window_height_px / 2;

	// Initialize the camera to center around the player
	camera_position.x = player_position.x - half_window_width;
	camera_position.y = player_position.y - half_window_height;

	// Clamp the camera position to the map boundaries to prevent showing black areas
	if (camera_position.x < 0) {
		camera_position.x = 0; // Left boundary
	}
	else if (camera_position.x + window_width_px > map_width_px) {
		camera_position.x = map_width_px - window_width_px; // Right boundary
	}

	if (camera_position.y < 0) {
		camera_position.y = 0; // Top boundary
	}
	else if (camera_position.y + window_height_px > map_height_px) {
		camera_position.y = map_height_px - window_height_px; // Bottom boundary
	}
}

mat3 RenderSystem::createOrthographicProjection(float left, float right, float top, float bottom) {
	float sx = 2.0f / (right - left);
	float sy = 2.0f / (top - bottom);
	float tx = -(right + left) / (right - left);
	float ty = -(top + bottom) / (top - bottom);
	return mat3(vec3(sx, 0.0f, 0.0f), vec3(0.0f, sy, 0.0f), vec3(tx, ty, 1.0f));
}


void RenderSystem::drawBoundingBox(Entity entity, const mat3& projection) {
	Motion& motion = registry.motions.get(entity);
	vec2 bounding_box = { abs(motion.bb.x), abs(motion.bb.y) };

	if (registry.tiles.has(entity)) {
		return;
	}

	vec2 top_left = vec2(-bounding_box.x / 2, -bounding_box.y / 2);
	vec2 top_right = vec2(bounding_box.x / 2, -bounding_box.y / 2);
	vec2 bottom_left = vec2(-bounding_box.x / 2, bounding_box.y / 2);
	vec2 bottom_right = vec2(bounding_box.x / 2, bounding_box.y / 2);



	float vertices[] = {
		top_left.x, top_left.y, 0.0f,
		top_right.x, top_right.y, 0.0f,
		bottom_right.x, bottom_right.y, 0.0f,
		bottom_left.x, bottom_left.y, 0.0f
	};
	
	GLuint box_program = effects[(GLuint)EFFECT_ASSET_ID::BOX];
	glUseProgram(box_program);
	gl_has_errors();


	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	gl_has_errors();


	GLint in_position_loc = glGetAttribLocation(box_program, "in_position");
	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	gl_has_errors();


	mat3 mat = { { 1.f, 0.f, 0.f }, { 0.f, 1.f, 0.f}, { 0.f, 0.f, 1.f} };
	//Transform transform1;
	//transform1.translate(motion.position);
	//transform1.rotate(motion.angle);

	Transform transform;
	vec2 render_position = motion.position - camera_position;
	transform.translate(render_position);

	GLuint transform_loc = glGetUniformLocation(box_program, "transform");
	glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&transform.mat);
	GLuint projection_loc = glGetUniformLocation(box_program, "projection");
	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection);

	GLuint in = glGetUniformLocation(box_program, "input_col");
	vec3 color;

	if (registry.collisions.has(entity)) {
		color = vec3(0.f, 1.f, 0.f);
	}
	else {
		color = vec3(1.f,0.f,0.0f);
	}
	glUniform3f(in, color.x, color.y, color.z);
	gl_has_errors();


	glLineWidth(3.0f);
	glDrawArrays(GL_LINE_LOOP, 0, 4);
	gl_has_errors();


	glDeleteBuffers(1, &vbo);
}




void RenderSystem::initUIVBO() {
	if (!ui_vbo_initialized) {
		glGenVertexArrays(1, &ui_vao);
		glGenBuffers(1, &ui_vbo);

		glBindVertexArray(ui_vao);

		glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);  // Reserve space for vertices

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);

		ui_vbo_initialized = true;
	}
}

void RenderSystem::renderText(std::string text, float x, float y, float scale, const glm::vec3& color, const glm::mat4& trans) {
	// Activate corresponding render state
	// enable blending or you will just get solid boxes instead of text
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(fontShaderProgram);
	gl_has_errors();
	// bind buffers
	glBindVertexArray(text_vao);
	glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

	//// release buffers
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	//glBindVertexArray(0);

	// get shader uniforms
	GLint textColor_location =
		glGetUniformLocation(fontShaderProgram, "textColor");
	glUniform3f(textColor_location, color.x, color.y, color.z);

	GLint transformLoc =
		glGetUniformLocation(fontShaderProgram, "transform");
	glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(trans));

	glBindVertexArray(text_vao);

	// Iterate through characters
	for (auto c = text.begin(); c != text.end(); c++) {
		Character ch = Characters[*c];

		float xpos = x + ch.bearing.x * scale;
		float ypos = y - (ch.size.y - ch.bearing.y) * scale;

		float w = ch.size.x * scale;
		float h = ch.size.y * scale;

		// Update VBO for each character
		float vertices[6][4] = {
			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos,     ypos,       0.0f, 1.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },

			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },
			{ xpos + w, ypos + h,   1.0f, 0.0f }
		};

		// Render glyph texture over quad
		glBindTexture(GL_TEXTURE_2D, ch.textureID);
		// Update content of VBO memory
		glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Render quad
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// Advance cursors for the next glyph
		x += (ch.advance >> 6) * scale; // Bitshift by 6 to get value in pixels (1/64th of a pixel unit)
	}
	//glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}


void RenderSystem::drawInventoryUI() {

	glm::vec2 draggedPosition = mousePosition - dragOffset;
	Inventory& player_inventory = registry.players.get(player).inventory;
	//  the inventory screen position and size
	vec2 screen_position = vec2(50.f, 50.f);
	vec2 screen_size = vec2(window_width_px - 100.f, window_height_px - 100.f);

	//vertices for UI_SCREEN and PLAYER_UPGRADE_SLOT
	TexturedVertex screen_vertices[4] = {
		{ vec3(screen_position.x, screen_position.y, 0.f), vec2(0.f, 0.f) },                  // Bottom-left
		{ vec3(screen_position.x + screen_size.x, screen_position.y, 0.f), vec2(1.f, 0.f) },  // Bottom-right
		{ vec3(screen_position.x + screen_size.x, screen_position.y + screen_size.y, 0.f), vec2(1.f, 1.f) }, // Top-right
		{ vec3(screen_position.x, screen_position.y + screen_size.y, 0.f), vec2(0.f, 1.f) }   // Top-left
	};

	// Activate the shader and bind VBO data
	glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED]);
	glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(screen_vertices), screen_vertices, GL_DYNAMIC_DRAW);
	gl_has_errors();

	// Set up vertex attributes for UI
	GLint in_position_loc = glGetAttribLocation(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED], "in_position");
	GLint in_texcoord_loc = glGetAttribLocation(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED], "in_texcoord");

	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
	glEnableVertexAttribArray(in_texcoord_loc);
	glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)sizeof(vec3));
	gl_has_errors();

	// Render UI_SCREEN texture
	GLuint ui_texture_id = texture_gl_handles[(GLuint)TEXTURE_ASSET_ID::UI_SCREEN];
	glBindTexture(GL_TEXTURE_2D, ui_texture_id);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	gl_has_errors();
	//	glActiveTexture(GL_TEXTURE0);
	//GLuint upgrade_slot_texture_id = texture_gl_handles[(GLuint)TEXTURE_ASSET_ID::PLAYER_UPGRADE_SLOT];
	//glBindTexture(GL_TEXTURE_2D, upgrade_slot_texture_id);
	//glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	//gl_has_errors();

	//GLuint player_avatar_texture_id = texture_gl_handles[(GLuint)TEXTURE_ASSET_ID::PLAYER_AVATAR];
	//glBindTexture(GL_TEXTURE_2D, player_avatar_texture_id);
	//glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	//gl_has_errors();


	// Define the position and size of the upgrade button
	vec2 upgrade_button_position = vec2(730.f, 310.f); 
	vec2 upgrade_button_size = vec2(100.f, 100.f); 

	// Check if the mouse is hovering over the upgrade button
	bool isHoveringUpgradeButton = (mousePosition.x >= upgrade_button_position.x &&
		mousePosition.x <= upgrade_button_position.x + upgrade_button_size.x &&
		mousePosition.y >= upgrade_button_position.y &&
		mousePosition.y <= upgrade_button_position.y + upgrade_button_size.y);

	// Choose the appropriate texture based on hover status
	GLuint upgrade_button_texture_id = texture_gl_handles[(GLuint)(isHoveringUpgradeButton ? TEXTURE_ASSET_ID::UPGRADE_BUTTON_HOVER : TEXTURE_ASSET_ID::UPGRADE_BUTTON)];

	// Define vertices for the upgrade button
	TexturedVertex upgrade_button_vertices[4] = {
		{ vec3(upgrade_button_position.x, upgrade_button_position.y, 0.f), vec2(0.f, 0.f) },  // Bottom-left (flipped)
		{ vec3(upgrade_button_position.x + upgrade_button_size.x, upgrade_button_position.y, 0.f), vec2(1.f, 0.f) }, // Bottom-right (flipped)
		{ vec3(upgrade_button_position.x + upgrade_button_size.x, upgrade_button_position.y + upgrade_button_size.y, 0.f), vec2(1.f, 1.f) }, // Top-right (flipped)
		{ vec3(upgrade_button_position.x, upgrade_button_position.y + upgrade_button_size.y, 0.f), vec2(0.f, 1.f) } // Top-left (flipped)
	};


	// Render the upgrade button
	glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(upgrade_button_vertices), upgrade_button_vertices, GL_DYNAMIC_DRAW);
	glBindTexture(GL_TEXTURE_2D, upgrade_button_texture_id);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	gl_has_errors();


	// Define and render armor slot
	vec2 armor_slot_position = vec2(620.f, 165.f);
	vec2 armor_slot_size = vec2(90.f, 90.f);
	TexturedVertex armor_slot_vertices[4] = {
	{ vec3(armor_slot_position.x, armor_slot_position.y, 0.f), vec2(0.f, 0.f) },  // Bottom-left
	{ vec3(armor_slot_position.x + armor_slot_size.x, armor_slot_position.y, 0.f), vec2(1.f, 0.f) }, // Bottom-right
	{ vec3(armor_slot_position.x + armor_slot_size.x, armor_slot_position.y + armor_slot_size.y, 0.f), vec2(1.f, 1.f) }, // Top-right
	{ vec3(armor_slot_position.x, armor_slot_position.y + armor_slot_size.y, 0.f), vec2(0.f, 1.f) } // Top-left
	};

	glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(armor_slot_vertices), armor_slot_vertices, GL_DYNAMIC_DRAW);
	GLuint armor_slot_texture_id = texture_gl_handles[(GLuint)TEXTURE_ASSET_ID::ARMOR_SLOT];
	glBindTexture(GL_TEXTURE_2D, armor_slot_texture_id);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	gl_has_errors();

	// Render item in armor slot if present
	Item armor_item = player_inventory.getArmorItem();
	if (!armor_item.name.empty()) {

		TEXTURE_ASSET_ID armor_item_texture_enum = getTextureIDFromItemName(armor_item.name);
		GLuint armor_item_texture_id = texture_gl_handles[(GLuint)armor_item_texture_enum];
		ivec2 original_size = texture_dimensions[(GLuint)armor_item_texture_enum];
		float scale_factor = std::min(armor_slot_size.x / original_size.x, armor_slot_size.y / original_size.y) * 0.8f;
		vec2 item_size = vec2(original_size.x, original_size.y) * scale_factor;
		vec2 item_position = armor_slot_position + (armor_slot_size - item_size) / 2.0f;

		// Render the item in the armor slot
		TexturedVertex armor_item_vertices[4] = {
		{ vec3(item_position.x, item_position.y, 0.f), vec2(0.f, 0.f) },                    // Bottom-left (flipped)
		{ vec3(item_position.x + item_size.x, item_position.y, 0.f), vec2(1.f, 0.f) },      // Bottom-right (flipped)
		{ vec3(item_position.x + item_size.x, item_position.y + item_size.y, 0.f), vec2(1.f, 1.f) }, // Top-right (flipped)
		{ vec3(item_position.x, item_position.y + item_size.y, 0.f), vec2(0.f, 1.f) }       // Top-left (flipped)
		};

		glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(armor_item_vertices), armor_item_vertices, GL_DYNAMIC_DRAW);
		glBindTexture(GL_TEXTURE_2D, armor_item_texture_id);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		gl_has_errors();


		std::string quantity_text = std::to_string(armor_item.quantity);
		glm::vec3 font_color = glm::vec3(1.0f, 1.0f, 1.0f);
		glm::mat4 font_trans = glm::mat4(1.0f);

		vec2 text_position = armor_slot_position + vec2(armor_slot_size.x - 22.f, armor_slot_size.y - 70.f);
		text_position.y = window_height_px - text_position.y;
		renderText(quantity_text, text_position.x, text_position.y, 0.5f, font_color, font_trans);


		glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED]);
		glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
		in_position_loc = glGetAttribLocation(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED], "in_position");
		in_texcoord_loc = glGetAttribLocation(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED], "in_texcoord");

		glEnableVertexAttribArray(in_position_loc);
		glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
		glEnableVertexAttribArray(in_texcoord_loc);
		glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)sizeof(vec3));
		gl_has_errors();
	}
	if (armor_item.name == "CompanionRobot" || armor_item.name == "IceRobot") {
		glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::COLOURED]);

		vec2 bar_start_position = vec2(730.f, 190.f);
		vec2 bar_size = vec2(100.f, 20.f);
		float bar_spacing = 55.f;

		float health_percentage = glm::clamp(static_cast<float>(armor_item.health) / 30.f, 0.0f, 1.0f);
		float attack_percentage = glm::clamp(static_cast<float>(armor_item.damage) / 20.0f, 0.0f, 1.0f);
		float speed_percentage = glm::clamp(static_cast<float>(armor_item.speed) / 150.0f, 0.0f, 1.0f);

		vec3 background_color = vec3(0.7f, 0.7f, 0.7f);

		struct AttributeBar {
			float percentage;
			vec2 position;
		};

		AttributeBar bars[] = {
			{ attack_percentage, bar_start_position },
			{ health_percentage, bar_start_position + vec2(0.f, bar_spacing) },
			{ speed_percentage, bar_start_position + vec2(0.f, 2 * bar_spacing) }
		};

		for (const auto& bar : bars) {
			vec2 filled_bar_size = vec2(bar_size.x * bar.percentage, bar_size.y);

			TexturedVertex background_bar_vertices[4] = {
				{ vec3(bar.position.x, bar.position.y, 0.f), vec2(0.f, 0.f) },
				{ vec3(bar.position.x + bar_size.x, bar.position.y, 0.f), vec2(1.f, 0.f) },
				{ vec3(bar.position.x + bar_size.x, bar.position.y + bar_size.y, 0.f), vec2(1.f, 1.f) },
				{ vec3(bar.position.x, bar.position.y + bar_size.y, 0.f), vec2(0.f, 1.f) }
			};

			TexturedVertex filled_bar_vertices[4] = {
				{ vec3(bar.position.x, bar.position.y, 0.f), vec2(0.f, 0.f) },
				{ vec3(bar.position.x + filled_bar_size.x, bar.position.y, 0.f), vec2(1.f, 0.f) },
				{ vec3(bar.position.x + filled_bar_size.x, bar.position.y + bar_size.y, 0.f), vec2(1.f, 1.f) },
				{ vec3(bar.position.x, bar.position.y + bar_size.y, 0.f), vec2(0.f, 1.f) }
			};

			glBindBuffer(GL_ARRAY_BUFFER, robot_healthbar_vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(background_bar_vertices), background_bar_vertices, GL_DYNAMIC_DRAW);

			GLint in_position_loc = glGetAttribLocation(effects[(GLuint)EFFECT_ASSET_ID::COLOURED], "in_position");
			glEnableVertexAttribArray(in_position_loc);
			glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);

			GLint color_uloc = glGetUniformLocation(effects[(GLuint)EFFECT_ASSET_ID::COLOURED], "fcolor");
			glUniform3fv(color_uloc, 1, (float*)&background_color);

			GLuint transform_loc = glGetUniformLocation(effects[(GLuint)EFFECT_ASSET_ID::COLOURED], "transform");
			GLuint projection_loc = glGetUniformLocation(effects[(GLuint)EFFECT_ASSET_ID::COLOURED], "projection");
			mat3 identity_transform = mat3(1.0f);
			glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&identity_transform);

			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

			glBufferData(GL_ARRAY_BUFFER, sizeof(filled_bar_vertices), filled_bar_vertices, GL_DYNAMIC_DRAW);

			vec3 bar_color = glm::mix(vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), bar.percentage);
			glUniform3fv(color_uloc, 1, (float*)&bar_color);

			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

			glDisableVertexAttribArray(in_position_loc);

			gl_has_errors();
		}
	}



	glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED]);
	glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
	 in_position_loc = glGetAttribLocation(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED], "in_position");
	 in_texcoord_loc = glGetAttribLocation(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED], "in_texcoord");

	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
	glEnableVertexAttribArray(in_texcoord_loc);
	glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)sizeof(vec3));
	gl_has_errors();
	// Define and render weapon slot
	vec2 weapon_slot_position = vec2(620.f, 260.f);
	vec2 weapon_slot_size = vec2(90.f, 90.f);
	TexturedVertex weapon_slot_vertices[4] = {
		{ vec3(weapon_slot_position.x, weapon_slot_position.y, 0.f), vec2(0.f, 0.f) },
		{ vec3(weapon_slot_position.x + weapon_slot_size.x, weapon_slot_position.y, 0.f), vec2(1.f, 0.f) },
		{ vec3(weapon_slot_position.x + weapon_slot_size.x, weapon_slot_position.y + weapon_slot_size.y, 0.f), vec2(1.f, 1.f) },
		{ vec3(weapon_slot_position.x, weapon_slot_position.y + weapon_slot_size.y, 0.f), vec2(0.f, 1.f) }
	};

	glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(weapon_slot_vertices), weapon_slot_vertices, GL_DYNAMIC_DRAW);
	GLuint weapon_slot_texture_id = texture_gl_handles[(GLuint)TEXTURE_ASSET_ID::WEAPON_SLOT];
	glBindTexture(GL_TEXTURE_2D, weapon_slot_texture_id);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	gl_has_errors();

	// Render item in weapon slot if present
	Item weapon_item = player_inventory.getWeaponItem();
	if (!weapon_item.name.empty()) {
		TEXTURE_ASSET_ID weapon_item_texture_enum = getTextureIDFromItemName(weapon_item.name);
		GLuint weapon_item_texture_id = texture_gl_handles[(GLuint)weapon_item_texture_enum];
		ivec2 original_size = texture_dimensions[(GLuint)weapon_item_texture_enum];
		float scale_factor = std::min(weapon_slot_size.x / original_size.x, weapon_slot_size.y / original_size.y) * 0.8f;
		vec2 item_size = vec2(original_size.x, original_size.y) * scale_factor;
		vec2 item_position = weapon_slot_position + (weapon_slot_size - item_size) / 2.0f;

		// Render the item in the armor slot
		TexturedVertex weapon_item_vertices[4] = {
		{ vec3(item_position.x, item_position.y, 0.f), vec2(0.f, 0.f) },                    // Bottom-left (flipped)
		{ vec3(item_position.x + item_size.x, item_position.y, 0.f), vec2(1.f, 0.f) },      // Bottom-right (flipped)
		{ vec3(item_position.x + item_size.x, item_position.y + item_size.y, 0.f), vec2(1.f, 1.f) }, // Top-right (flipped)
		{ vec3(item_position.x, item_position.y + item_size.y, 0.f), vec2(0.f, 1.f) }       // Top-left (flipped)
		};

		glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(weapon_item_vertices), weapon_item_vertices, GL_DYNAMIC_DRAW);
		glBindTexture(GL_TEXTURE_2D, weapon_item_texture_id);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		gl_has_errors();

		std::string quantity_text = std::to_string(weapon_item.quantity);
		glm::vec3 font_color = glm::vec3(1.0f, 1.0f, 1.0f);
		glm::mat4 font_trans = glm::mat4(1.0f);

		vec2 text_position = weapon_slot_position + vec2(weapon_slot_size.x - 22.f, weapon_slot_size.y - 70.f);
		text_position.y = window_height_px - text_position.y;
		renderText(quantity_text, text_position.x, text_position.y, 0.5f, font_color, font_trans);


		glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED]);
		glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
		in_position_loc = glGetAttribLocation(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED], "in_position");
		in_texcoord_loc = glGetAttribLocation(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED], "in_texcoord");

		glEnableVertexAttribArray(in_position_loc);
		glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
		glEnableVertexAttribArray(in_texcoord_loc);
		glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)sizeof(vec3));
		gl_has_errors();
	}
	// Inventory Slots Configuration (2 rows x 5 columns)
	vec2 slot_size = vec2(90.f, 90.f);
	float horizontal_spacing = 5.f;
	float vertical_spacing = 20.f;

	vec2 slots_start_position = vec2(
		screen_position.x + (screen_size.x - (5 * slot_size.x + 4 * horizontal_spacing)) / 2,
		screen_position.y + (screen_size.y) - 260.f
	);
	// Access player's inventory items
	const auto& items = player_inventory.getItems();

	// Draw 10 inventory slots in a 2x5 grid, excluding the armor slot
	for (int slot_index = 0; slot_index < 10; ++slot_index) {
		vec2 current_slot_position = slots_start_position + vec2(
			(slot_index % 5) * (slot_size.x + horizontal_spacing),
			(slot_index / 5) * (slot_size.y + vertical_spacing - 10.f)
		);

		// Define vertices for each slot
		TexturedVertex slot_vertices[4] = {
			{ vec3(current_slot_position.x, current_slot_position.y, 0.f), vec2(0.f, 1.f) },
			{ vec3(current_slot_position.x + slot_size.x, current_slot_position.y, 0.f), vec2(1.f, 1.f) },
			{ vec3(current_slot_position.x + slot_size.x, current_slot_position.y + slot_size.y, 0.f), vec2(1.f, 0.f) },
			{ vec3(current_slot_position.x, current_slot_position.y + slot_size.y, 0.f), vec2(0.f, 0.f) }
		};

		glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(slot_vertices), slot_vertices, GL_DYNAMIC_DRAW);
		GLuint slot_texture_id = texture_gl_handles[(GLuint)TEXTURE_ASSET_ID::INV_SLOT];
		glBindTexture(GL_TEXTURE_2D, slot_texture_id);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		gl_has_errors();

		// Draw item in slot if it’s not being dragged
		if (!(isDragging && draggedSlot == slot_index)) {
			Item item = player_inventory.slots[slot_index].item;
			if (!item.name.empty()) {
				TEXTURE_ASSET_ID item_texture_id = getTextureIDFromItemName(item.name);
				renderInventoryItem(item, current_slot_position, slot_size);
				std::string quantity_text = std::to_string(item.quantity);
				glm::vec3 font_color = glm::vec3(1.0f, 1.0f, 1.0f);
				glm::mat4 font_trans = glm::mat4(1.0f);

				vec2 text_position = current_slot_position + vec2(slot_size.x - 22.f, slot_size.y - 70.f);
				text_position.y = window_height_px - text_position.y;
				renderText(quantity_text, text_position.x, text_position.y, 0.5f, font_color, font_trans);


				glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED]);
				glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
				in_position_loc = glGetAttribLocation(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED], "in_position");
				in_texcoord_loc = glGetAttribLocation(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED], "in_texcoord");

				glEnableVertexAttribArray(in_position_loc);
				glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
				glEnableVertexAttribArray(in_texcoord_loc);
				glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)sizeof(vec3));
				gl_has_errors();
			}
		}

	}

	// Render dragged item at dragged position
	if (isDragging && draggedSlot != -1) {
		renderInventoryItem(player_inventory.slots[draggedSlot].item, draggedPosition, slot_size);
	}
	float text_scale = 0.5f;
	glm::vec3 font_color = glm::vec3(1.0f, 1.0f, 1.0f); // White color
	glm::mat4 font_trans = glm::mat4(1.0f); // Identity matrix


	float health_text_x = 420.0f;
	float health_text_y = 400.f;

	// Load the font and render the text
	std::string text = std::to_string((int)registry.players.get(player).armor_stat);
	renderText("Armor: " + text, health_text_x, health_text_y, text_scale, font_color, font_trans);
	std::string weapon_text = std::to_string((int)registry.players.get(player).weapon_stat);
	renderText("Weapon: " + weapon_text, health_text_x, 375.f, text_scale, font_color, font_trans);


}

void RenderSystem::renderInventoryItem(const Item& item, const vec2& position, const vec2& size) {

	TEXTURE_ASSET_ID item_texture_enum = getTextureIDFromItemName(item.name);
	GLuint item_texture_id = texture_gl_handles[(GLuint)item_texture_enum];

	float scale_factor = std::min(size.x / texture_dimensions[(GLuint)item_texture_enum].x,
		size.y / texture_dimensions[(GLuint)item_texture_enum].y);
	vec2 item_size = vec2(texture_dimensions[(GLuint)item_texture_enum]) * scale_factor * 0.7f;

	vec2 item_position = position + (size - item_size) / 2.0f; // Center item within slot or dragged position

	TexturedVertex item_vertices[4] = {
	 { vec3(item_position.x, item_position.y, 0.f), vec2(0.f, 0.f) },                  // Bottom-left (flipped)
	 { vec3(item_position.x + item_size.x, item_position.y, 0.f), vec2(1.f, 0.f) },    // Bottom-right (flipped)
	 { vec3(item_position.x + item_size.x, item_position.y + item_size.y, 0.f), vec2(1.f, 1.f) }, // Top-right
	 { vec3(item_position.x, item_position.y + item_size.y, 0.f), vec2(0.f, 1.f) }     // Top-left
	};


	glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(item_vertices), item_vertices, GL_DYNAMIC_DRAW);
	glBindTexture(GL_TEXTURE_2D, item_texture_id);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	gl_has_errors();
}

vec2 RenderSystem::getSlotPosition(int slot_index) const {
		vec2 slot_size = vec2(90.f, 90.f);
	float horizontal_spacing = 5.f;
	float vertical_spacing = 20.f;

	vec2 screen_position = vec2(50.f, 50.f);
	vec2 screen_size = vec2(window_width_px - 100.f, window_height_px - 100.f);
	vec2 slots_start_position = vec2(
		screen_position.x + (screen_size.x - (5 * slot_size.x + 4 * horizontal_spacing)) / 2,
		screen_position.y + (screen_size.y) - 275.f
	);

	int row = slot_index / 5;
	int col = slot_index % 5;

	vec2 current_slot_position = slots_start_position + vec2(
		col * (slot_size.x + horizontal_spacing),
		row * (slot_size.y + vertical_spacing)
	);

	return current_slot_position;
}



void RenderSystem::drawReactionBox(Entity entity, const mat3& projection) {
	Motion& motion = registry.motions.get(entity);

	if (!registry.robots.has(entity)) {
		return;
	}

	Robot r = registry.robots.get(entity);
	std::vector<vec2> boxs = { r.search_box,r.attack_box,r.panic_box };



	for (vec2 bounding_box : boxs) {
		vec2 top_left = vec2(-bounding_box.x / 2, -bounding_box.y / 2);
		vec2 top_right = vec2(bounding_box.x / 2, -bounding_box.y / 2);
		vec2 bottom_left = vec2(-bounding_box.x / 2, bounding_box.y / 2);
		vec2 bottom_right = vec2(bounding_box.x / 2, bounding_box.y / 2);



		float vertices[] = {
			top_left.x, top_left.y, 0.0f,
			top_right.x, top_right.y, 0.0f,
			bottom_right.x, bottom_right.y, 0.0f,
			bottom_left.x, bottom_left.y, 0.0f
		};

		GLuint box_program = effects[(GLuint)EFFECT_ASSET_ID::BOX];
		glUseProgram(box_program);
		gl_has_errors();


		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
		gl_has_errors();


		GLint in_position_loc = glGetAttribLocation(box_program, "in_position");
		glEnableVertexAttribArray(in_position_loc);
		glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		gl_has_errors();


		Transform transform;
		vec2 render_position = motion.position - camera_position;
		transform.translate(render_position);

		GLuint transform_loc = glGetUniformLocation(box_program, "transform");
		glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&transform.mat);
		GLuint projection_loc = glGetUniformLocation(box_program, "projection");
		glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection);

		GLuint in = glGetUniformLocation(box_program, "input_col");
		vec3 color;

		if (registry.collisions.has(entity)) {
			color = vec3(0.f, 1.f, 0.f);
		}
		else {
			color = vec3(1.f, 0.f, 0.0f);
		}
		glUniform3f(in, color.x, color.y, color.z);
		gl_has_errors();


		glLineWidth(3.0f);
		glDrawArrays(GL_LINE_LOOP, 0, 4);
		gl_has_errors();


		glDeleteBuffers(1, &vbo);
	}
}


void RenderSystem::drawBossReactionBox(Entity entity, const mat3& projection) {
	Motion& motion = registry.motions.get(entity);

	if (!registry.bossRobots.has(entity)) {
		return;
	}

	BossRobot r = registry.bossRobots.get(entity);
	std::vector<vec2> boxs = { r.search_box,r.attack_box,r.panic_box };

	for (vec2 bounding_box : boxs) {
		vec2 top_left = vec2(-bounding_box.x / 2, -bounding_box.y / 2);
		vec2 top_right = vec2(bounding_box.x / 2, -bounding_box.y / 2);
		vec2 bottom_left = vec2(-bounding_box.x / 2, bounding_box.y / 2);
		vec2 bottom_right = vec2(bounding_box.x / 2, bounding_box.y / 2);



		float vertices[] = {
			top_left.x, top_left.y, 0.0f,
			top_right.x, top_right.y, 0.0f,
			bottom_right.x, bottom_right.y, 0.0f,
			bottom_left.x, bottom_left.y, 0.0f
		};

		GLuint box_program = effects[(GLuint)EFFECT_ASSET_ID::BOX];
		glUseProgram(box_program);
		gl_has_errors();


		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
		gl_has_errors();


		GLint in_position_loc = glGetAttribLocation(box_program, "in_position");
		glEnableVertexAttribArray(in_position_loc);
		glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		gl_has_errors();


		Transform transform;
		vec2 render_position = motion.position - camera_position;
		transform.translate(render_position);

		GLuint transform_loc = glGetUniformLocation(box_program, "transform");
		glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&transform.mat);
		GLuint projection_loc = glGetUniformLocation(box_program, "projection");
		glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection);

		GLuint in = glGetUniformLocation(box_program, "input_col");
		vec3 color;

		if (registry.collisions.has(entity)) {
			color = vec3(0.f, 1.f, 0.f);
		}
		else {
			color = vec3(1.f, 0.f, 0.0f);
		}
		glUniform3f(in, color.x, color.y, color.z);
		gl_has_errors();


		glLineWidth(3.0f);
		glDrawArrays(GL_LINE_LOOP, 0, 4);
		gl_has_errors();


		glDeleteBuffers(1, &vbo);
	}
}


void RenderSystem::updateFPS() {
	Uint32 current_time = SDL_GetTicks();
	frame_count++;

	// Update FPS every second
	if (current_time - last_time >= 1000) {
		fps = frame_count / ((current_time - last_time) / 1000.0f); // Calculate FPS
		last_time = current_time;
		frame_count = 0;
		// debug_fps
		//std::cout << "FPS: " << fps << std::endl;
	}
}

void RenderSystem::drawFPSCounter(const mat3& projection) {
	// Set FPS position and scale
	float fps_x = 20.0f;
	float fps_y = window_height_px - 40.0f; 
	float text_scale = 0.8f;
	glm::vec3 font_color(0.0f, 1.0f, 0.0f); 

	// Convert FPS to string for display
	std::string fps_text = "FPS: " + std::to_string(static_cast<int>(fps));
	glm::mat4 font_trans = glm::mat4(1.0f); 
	renderText(fps_text, fps_x, fps_y, text_scale, font_color, font_trans);
}

void RenderSystem::drawRobotHealthBar(Entity robot, const mat3& projection) {
	if (!registry.robots.has(robot) || !robot_healthbar_vbo_initialized)
		return;

	// Bind the shader program for coloring
	glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::COLOURED]);
	gl_has_errors();

	// Retrieve the robot�s health information
	Robot& robot_data = registry.robots.get(robot);
	float health_percentage = robot_data.current_health / robot_data.max_health;
	health_percentage = glm::clamp(health_percentage, 0.0f, 1.0f);
	// Position the health bar above the robot and apply camera offset
	Motion& motion = registry.motions.get(robot);
	float vertical_offset = motion.scale.y * -0.3f; 
	vec2 bar_position; 
	if (registry.iceRobotAnimations.has(robot)) {
		bar_position = motion.position - camera_position + vec2(0.0f, vertical_offset - 20.f);
	}
	else {
		bar_position = motion.position - camera_position + vec2(0.0f, vertical_offset);
	}
	vec2 bar_size = vec2(30.f, 5.f);

	TexturedVertex full_bar_vertices[4] = {
		{ vec3(bar_position.x - bar_size.x / 2, bar_position.y, 0.f), vec2(0.f, 1.f) },
		{ vec3(bar_position.x + bar_size.x / 2, bar_position.y, 0.f), vec2(1.f, 1.f) },
		{ vec3(bar_position.x + bar_size.x / 2, bar_position.y + bar_size.y, 0.f), vec2(1.f, 0.f) },
		{ vec3(bar_position.x - bar_size.x / 2, bar_position.y + bar_size.y, 0.f), vec2(0.f, 0.f) }
	};

	// Bind the VBO and load vertex data
	glBindBuffer(GL_ARRAY_BUFFER, robot_healthbar_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(full_bar_vertices), full_bar_vertices, GL_DYNAMIC_DRAW);
	gl_has_errors();

	// Get and enable position attribute location
	GLint in_position_loc = glGetAttribLocation(effects[(GLuint)EFFECT_ASSET_ID::COLOURED], "in_position");
	if (in_position_loc == -1) {
		std::cerr << "Error: Position attribute not found in shader" << std::endl;
		return;
	}
	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
	gl_has_errors();

	// Set the color for the background bar
	GLint color_uloc = glGetUniformLocation(effects[(GLuint)EFFECT_ASSET_ID::COLOURED], "fcolor");
	vec3 background_color = vec3(0.7f, 0.7f, 0.7f);  // Gray
	glUniform3fv(color_uloc, 1, (float*)&background_color);

	// Use projection matrix for screen positioning
	GLuint transform_loc = glGetUniformLocation(effects[(GLuint)EFFECT_ASSET_ID::COLOURED], "transform");
	GLuint projection_loc = glGetUniformLocation(effects[(GLuint)EFFECT_ASSET_ID::COLOURED], "projection");
	mat3 identity_transform = mat3(1.0f);  // No extra transformations
	glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&identity_transform);
	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection);

	// Draw the background bar
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	// vertices for the filled health bar portion
	TexturedVertex health_bar_vertices[4] = {
		{ vec3(bar_position.x - bar_size.x / 2, bar_position.y, 0.f), vec2(0.f, 1.f) },
		{ vec3(bar_position.x - bar_size.x / 2 + bar_size.x * health_percentage, bar_position.y, 0.f), vec2(1.f, 1.f) },
		{ vec3(bar_position.x - bar_size.x / 2 + bar_size.x * health_percentage, bar_position.y + bar_size.y, 0.f), vec2(1.f, 0.f) },
		{ vec3(bar_position.x - bar_size.x / 2, bar_position.y + bar_size.y, 0.f), vec2(0.f, 0.f) }
	};

	// Update VBO data and set health color
	glBufferData(GL_ARRAY_BUFFER, sizeof(health_bar_vertices), health_bar_vertices, GL_DYNAMIC_DRAW);
	vec3 health_color = glm::mix(vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), health_percentage);
	glUniform3fv(color_uloc, 1, (float*)&health_color);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glDisableVertexAttribArray(in_position_loc);
}



void RenderSystem::initRobotHealthBarVBO() {
	if (!robot_healthbar_vbo_initialized) {
		glGenVertexArrays(1, &robot_healthbar_vao);
		glGenBuffers(1, &robot_healthbar_vbo);

		glBindVertexArray(robot_healthbar_vao);

		glBindBuffer(GL_ARRAY_BUFFER, robot_healthbar_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);

		robot_healthbar_vbo_initialized = true;
	}
}

void RenderSystem::drawBossRobotHealthBar(Entity boss_robot, const mat3& projection) {
	if ( !robot_healthbar_vbo_initialized) {
		return;
	}
	if (!registry.bossRobots.has(boss_robot)) {
		return;
	}
	// Bind the shader program for coloring
	glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::COLOURED]);
	gl_has_errors();

	// Retrieve the robot�s health information
	BossRobot& robot_data = registry.bossRobots.get(boss_robot);
	float health_percentage = robot_data.current_health / robot_data.max_health;
	health_percentage = glm::clamp(health_percentage, 0.0f, 1.0f);
	// Position the health bar above the robot and apply camera offset
	Motion& motion = registry.motions.get(boss_robot);
	float vertical_offset = motion.scale.y * -0.3f - 50.0f; 
	vec2 bar_position = motion.position - camera_position + vec2(0.0f, vertical_offset);
	vec2 bar_size = vec2(150.f, 20.f); 

	TexturedVertex full_bar_vertices[4] = {
		{ vec3(bar_position.x - bar_size.x / 2, bar_position.y, 0.f), vec2(0.f, 1.f) },
		{ vec3(bar_position.x + bar_size.x / 2, bar_position.y, 0.f), vec2(1.f, 1.f) },
		{ vec3(bar_position.x + bar_size.x / 2, bar_position.y + bar_size.y, 0.f), vec2(1.f, 0.f) },
		{ vec3(bar_position.x - bar_size.x / 2, bar_position.y + bar_size.y, 0.f), vec2(0.f, 0.f) }
	};

	// Bind the VBO and load vertex data
	glBindBuffer(GL_ARRAY_BUFFER, robot_healthbar_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(full_bar_vertices), full_bar_vertices, GL_DYNAMIC_DRAW);
	gl_has_errors();

	// Get and enable position attribute location
	GLint in_position_loc = glGetAttribLocation(effects[(GLuint)EFFECT_ASSET_ID::COLOURED], "in_position");
	if (in_position_loc == -1) {
		std::cerr << "Error: Position attribute not found in shader" << std::endl;
		return;
	}
	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
	gl_has_errors();

	// Set the color for the background bar
	GLint color_uloc = glGetUniformLocation(effects[(GLuint)EFFECT_ASSET_ID::COLOURED], "fcolor");
	vec3 background_color = vec3(0.7f, 0.7f, 0.7f);  // Gray
	glUniform3fv(color_uloc, 1, (float*)&background_color);

	// Use projection matrix for screen positioning
	GLuint transform_loc = glGetUniformLocation(effects[(GLuint)EFFECT_ASSET_ID::COLOURED], "transform");
	GLuint projection_loc = glGetUniformLocation(effects[(GLuint)EFFECT_ASSET_ID::COLOURED], "projection");
	mat3 identity_transform = mat3(1.0f);  // No extra transformations
	glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&identity_transform);
	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection);

	// Draw the background bar
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	// vertices for the filled health bar portion
	TexturedVertex health_bar_vertices[4] = {
		{ vec3(bar_position.x - bar_size.x / 2, bar_position.y, 0.f), vec2(0.f, 1.f) },
		{ vec3(bar_position.x - bar_size.x / 2 + bar_size.x * health_percentage, bar_position.y, 0.f), vec2(1.f, 1.f) },
		{ vec3(bar_position.x - bar_size.x / 2 + bar_size.x * health_percentage, bar_position.y + bar_size.y, 0.f), vec2(1.f, 0.f) },
		{ vec3(bar_position.x - bar_size.x / 2, bar_position.y + bar_size.y, 0.f), vec2(0.f, 0.f) }
	};

	// Update VBO data and set health color
	glBufferData(GL_ARRAY_BUFFER, sizeof(health_bar_vertices), health_bar_vertices, GL_DYNAMIC_DRAW);
	vec3 health_color = glm::mix(vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), health_percentage);
	glUniform3fv(color_uloc, 1, (float*)&health_color);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glDisableVertexAttribArray(in_position_loc);
}


void RenderSystem::renderCaptureUI(const Robot& robot, Entity entity) {
	currentRobotEntity = entity;
	//  the inventory screen position and size

	vec2 screen_position = vec2(50.f, 50.f);
	vec2 screen_size = vec2(window_width_px - 100.f, window_height_px - 100.f);

	TexturedVertex screen_vertices[4] = {
		{ vec3(screen_position.x, screen_position.y, 0.f), vec2(0.f, 0.f) },                  // Bottom-left
		{ vec3(screen_position.x + screen_size.x, screen_position.y, 0.f), vec2(1.f, 0.f) },  // Bottom-right
		{ vec3(screen_position.x + screen_size.x, screen_position.y + screen_size.y, 0.f), vec2(1.f, 1.f) }, // Top-right
		{ vec3(screen_position.x, screen_position.y + screen_size.y, 0.f), vec2(0.f, 1.f) }   // Top-left
	};

	glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED]);
	glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(screen_vertices), screen_vertices, GL_DYNAMIC_DRAW);
	gl_has_errors();

	GLint in_position_loc = glGetAttribLocation(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED], "in_position");
	GLint in_texcoord_loc = glGetAttribLocation(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED], "in_texcoord");

	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
	glEnableVertexAttribArray(in_texcoord_loc);
	glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)sizeof(vec3));
	gl_has_errors();

	GLuint ui_texture_id = texture_gl_handles[(GLuint)TEXTURE_ASSET_ID::CAPTURE_UI];
	glBindTexture(GL_TEXTURE_2D, ui_texture_id);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	gl_has_errors();

	vec2 crocbot_position = vec2((window_width_px - 210.f) / 2.f, (window_height_px - 120.f) / 2.f); // Centered position
	vec2 crocbot_size = vec2(300.f, 200.f) * 0.9f;

	TexturedVertex crocbot_vertices[4] = {
		{ vec3(crocbot_position.x, crocbot_position.y, 0.f), vec2(0.f, 0.f) },
		{ vec3(crocbot_position.x + crocbot_size.x, crocbot_position.y, 0.f), vec2(1.f, 0.f) },
		{ vec3(crocbot_position.x + crocbot_size.x, crocbot_position.y + crocbot_size.y, 0.f), vec2(1.f, 1.f) },
		{ vec3(crocbot_position.x, crocbot_position.y + crocbot_size.y, 0.f), vec2(0.f, 1.f) }
	};
	glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(crocbot_vertices), crocbot_vertices, GL_DYNAMIC_DRAW);
	if (registry.iceRobotAnimations.has(currentRobotEntity)) {
		GLuint crockbot_texture = texture_gl_handles[(GLuint)TEXTURE_ASSET_ID::ICE_ROBOT];
		glBindTexture(GL_TEXTURE_2D, crockbot_texture);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		gl_has_errors();
	}
	else {
	GLuint crockbot_texture = texture_gl_handles[(GLuint)TEXTURE_ASSET_ID::COMPANION_CROCKBOT];
	glBindTexture(GL_TEXTURE_2D, crockbot_texture);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	gl_has_errors();
}

	renderButton(vec2(850.f, 410.f), vec2(110.f, 110.f), TEXTURE_ASSET_ID::C_BUTTON, TEXTURE_ASSET_ID::C_BUTTON_HOVER, mousePosition);
	renderButton(vec2(375.f, 410.f), vec2(110.f, 110.f), TEXTURE_ASSET_ID::D_BUTTON, TEXTURE_ASSET_ID::D_BUTTON_HOVER, mousePosition);
	
	vec2 start_position = vec2(335.f, 270.f); 
	vec2 item_size = vec2(50.f, 50.f);      
	float horizontal_spacing = 80.f;      
	float vertical_spacing = 30.f;     
	int items_per_row = 2;           
	// First loop: Render item icons
	for (size_t i = 0; i < robot.disassembleItems.size(); ++i) {
		const Item& item = robot.disassembleItems[i];

		vec2 item_position = start_position + vec2(0.f, i * (item_size.y + vertical_spacing));

		TEXTURE_ASSET_ID item_texture_id = getTextureIDFromItemName(item.name);

		if (item_texture_id == TEXTURE_ASSET_ID::TEXTURE_COUNT) {
			std::cerr << "Error: No texture found for item " << item.name << std::endl;
			continue;
		}

		GLuint texture_id = texture_gl_handles[(GLuint)item_texture_id];
		if (!texture_id) {
			std::cerr << "Error: Texture ID not found for item " << item.name << std::endl;
			continue;
		}

		TexturedVertex item_vertices[4] = {
			{ vec3(item_position.x, item_position.y, 0.f), vec2(0.f, 0.f) },
			{ vec3(item_position.x + item_size.x, item_position.y, 0.f), vec2(1.f, 0.f) },
			{ vec3(item_position.x + item_size.x, item_position.y + item_size.y, 0.f), vec2(1.f, 1.f) },
			{ vec3(item_position.x, item_position.y + item_size.y, 0.f), vec2(0.f, 1.f) }
		};

		glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(item_vertices), item_vertices, GL_DYNAMIC_DRAW);
		glBindTexture(GL_TEXTURE_2D, texture_id);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}
	for (size_t i = 0; i < robot.disassembleItems.size(); ++i) {
		const Item& item = robot.disassembleItems[robot.disassembleItems.size() - 1 - i]; // Access items in reverse order

		vec2 item_position = start_position + vec2(-10.f, i * (item_size.y + 25.0f)) + vec2(0.f, 70.0f); // Shift upwards by 20.0f

		std::string quantity_text = std::to_string(item.quantity) + "x " + item.name;
		renderText(quantity_text, item_position.x + 65.f, item_position.y, 0.4f, vec3(1.0f, 1.0f, 1.0f), mat4(1.0f));
	}




	renderStatBar(vec2(830.f, 270.f), vec2(150.f, 20.f), robot.attack, robot.max_attack);
	renderStatBar(vec2(830.f, 320.f), vec2(150.f, 20.f), robot.current_health, robot.max_health);
	renderStatBar(vec2(830.f, 370.f), vec2(150.f, 20.f), robot.speed, robot.max_speed);
	
}
void RenderSystem::renderStatBar(const vec2& bar_position, const vec2& bar_size, float current_value, float max_value) {

	float percentage = std::max(0.0f, std::min(current_value / max_value, 1.0f));

	//std::cout << "Current Value: " << current_value
	//	<< ", Max Value: " << max_value
	//	<< ", Percentage: " << percentage << std::endl;

	glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::COLOURED]);

	TexturedVertex full_bar_vertices[4] = {
		{ vec3(bar_position.x, bar_position.y, 0.f), vec2(0.f, 1.f) },
		{ vec3(bar_position.x + bar_size.x, bar_position.y, 0.f), vec2(1.f, 1.f) },
		{ vec3(bar_position.x + bar_size.x, bar_position.y + bar_size.y, 0.f), vec2(1.f, 0.f) },
		{ vec3(bar_position.x, bar_position.y + bar_size.y, 0.f), vec2(0.f, 0.f) }
	};

	glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(full_bar_vertices), full_bar_vertices, GL_DYNAMIC_DRAW);

	GLint in_position_loc = glGetAttribLocation(effects[(GLuint)EFFECT_ASSET_ID::COLOURED], "in_position");
	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);

	GLint color_uloc = glGetUniformLocation(effects[(GLuint)EFFECT_ASSET_ID::COLOURED], "fcolor");
	vec3 background_color = vec3(0.7f, 0.7f, 0.7f);
	glUniform3fv(color_uloc, 1, (float*)&background_color);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	TexturedVertex filled_bar_vertices[4] = {
		{ vec3(bar_position.x, bar_position.y, 0.f), vec2(0.f, 1.f) },
		{ vec3(bar_position.x + bar_size.x * percentage, bar_position.y, 0.f), vec2(percentage, 1.f) },
		{ vec3(bar_position.x + bar_size.x * percentage, bar_position.y + bar_size.y, 0.f), vec2(percentage, 0.f) },
		{ vec3(bar_position.x, bar_position.y + bar_size.y, 0.f), vec2(0.f, 0.f) }
	};

	glBufferData(GL_ARRAY_BUFFER, sizeof(filled_bar_vertices), filled_bar_vertices, GL_DYNAMIC_DRAW);

	vec3 filled_color = glm::mix(vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), percentage);
	glUniform3fv(color_uloc, 1, (float*)&filled_color);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glDisableVertexAttribArray(in_position_loc);

	std::string stat_text = std::to_string(static_cast<int>(current_value)) + "/" + std::to_string(static_cast<int>(max_value));
	vec3 text_color = vec3(1.0f, 1.0f, 1.0f);  // White
	float text_scale = 0.4f;

	float text_width = stat_text.length() * 7.0f * text_scale;
	renderText(stat_text,
		bar_position.x + bar_size.x - text_width - 30.0f,
		bar_position.y + 75.0f + text_width,
		text_scale, text_color, mat4(1.0f));
}



void RenderSystem::renderButton(const vec2& position, const vec2& size, TEXTURE_ASSET_ID texture_id, TEXTURE_ASSET_ID hover_texture_id, const vec2& mouse_position) {
	// Check if the mouse is over the button
	bool is_hovered = mouse_position.x >= position.x && mouse_position.x <= (position.x + size.x) &&
		mouse_position.y >= position.y && mouse_position.y <= (position.y + size.y);

	// Use the hover texture if hovered, otherwise use the normal texture
	GLuint button_texture_id = is_hovered ? texture_gl_handles[(GLuint)hover_texture_id] : texture_gl_handles[(GLuint)texture_id];

	TexturedVertex button_vertices[4] = {
		{ vec3(position.x, position.y, 0.f), vec2(0.f, 0.f) },
		{ vec3(position.x + size.x, position.y, 0.f), vec2(1.f, 0.f) },
		{ vec3(position.x + size.x, position.y + size.y, 0.f), vec2(1.f, 1.f) },
		{ vec3(position.x, position.y + size.y, 0.f), vec2(0.f, 1.f) }
	};

	glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(button_vertices), button_vertices, GL_DYNAMIC_DRAW);
	glBindTexture(GL_TEXTURE_2D, button_texture_id);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	gl_has_errors();
}


void RenderSystem::renderStartScreen() {
	glm::mat4 default_transform = glm::mat4(1.0f);
	std::string instruction_text = "Press G to start";
	renderText(instruction_text, window_width_px / 2 - 190.0f, window_height_px / 2 - 200.0f, 1.0f, glm::vec3(1.0f, 1.0f, 0.0f), default_transform);
}

void RenderSystem::initStartScreenVBO() {
	if (!startscreen_vbo_initialized) {
		glGenVertexArrays(1, &startscreen_vao);
		glGenBuffers(1, &startscreen_vbo);

		glBindVertexArray(startscreen_vao);

		float screen_vertices[] = {
			-1.0f, -1.0f,  0.0f, 0.0f, 
			 1.0f, -1.0f,  1.0f, 0.0f,
			-1.0f,  1.0f,  1.0f, 1.0f,
			 1.0f,  1.0f,  0.0f, 1.0f
		};


		glBindBuffer(GL_ARRAY_BUFFER, startscreen_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(screen_vertices), screen_vertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);


		startscreen_vbo_initialized = true;
	}
}


void RenderSystem::startCutscene(const std::vector<TEXTURE_ASSET_ID>& images) {
	cutscene_images = images;
	current_cutscene_index = 0;
	cutscene_timer = 0.f;
	playing_cutscene = true;

	initCutsceneVBO();
}

//void RenderSystem::updateCutscene(float elapsed_time) {
//	if (!playing_cutscene) 
//		return;
//
//	cutscene_timer += elapsed_time / 1000.f;
//
//	if (cutscene_timer >= cutscene_duration_per_image) {
//		cutscene_timer = 0.f;
//		current_cutscene_index++;
//
//		if (current_cutscene_index >= cutscene_images.size()) {
//			playing_cutscene = false;
//			current_cutscene_index = 0;
//			cutscene_images.clear();
//		}
//	}
//}

void RenderSystem::initCutsceneVBO() {
	if (!cutscene_vbo_initialized) {
		glGenVertexArrays(1, &cutscene_vao);
		glGenBuffers(1, &cutscene_vbo);

		glBindVertexArray(cutscene_vao);

		float screen_vertices[] = {
			-1.0f, -1.0f,  0.0f, 0.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,
			-1.0f,  1.0f,  0.0f, 1.0f,
			 1.0f,  1.0f,  1.0f, 1.0f
		};

		glBindBuffer(GL_ARRAY_BUFFER, cutscene_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(screen_vertices), screen_vertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);

		cutscene_vbo_initialized = true;
	}
}

void RenderSystem::renderCutscene() {
	if (!playing_cutscene) {
		return;
	}

	// Safeguard: Ensure there are cutscene images
	if (cutscene_images.empty()) {
		std::cerr << "Cutscene images are empty. Ending cutscene rendering." << std::endl;
		playing_cutscene = false; // End the cutscene
		return;
	}

	// Ensure the index is within bounds
	if (current_cutscene_index >= cutscene_images.size()) {
		std::cerr << "Current cutscene index out of bounds. Resetting cutscene." << std::endl;
		playing_cutscene = false; // End the cutscene
		return;
	}

	int w, h;
	glfwGetFramebufferSize(window, &w, &h);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0.f, 0.f, 0.f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::SCREEN]);
	gl_has_errors();

	GLuint texture_id = texture_gl_handles[(GLuint)cutscene_images[current_cutscene_index]];
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	gl_has_errors();

	glBindVertexArray(cutscene_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	gl_has_errors();
}
