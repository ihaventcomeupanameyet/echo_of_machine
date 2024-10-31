// internal
#include "render_system.hpp"
#include <SDL.h>
#include "tileset.hpp"

#include "tiny_ecs_registry.hpp"
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

	// Set up vertex attributes
	GLint in_position_loc = glGetAttribLocation(program, "in_position");
	GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
	gl_has_errors();
	assert(in_texcoord_loc >= 0);

	glEnableVertexAttribArray(in_position_loc);
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



// draw the intermediate texture to the screen, with some distortion to simulate
void RenderSystem::drawToScreen()
{
	// Setting shaders
	// get the screen texture, sprite mesh, and program
	glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::SCREEN]);
	gl_has_errors();
	// Clearing backbuffer
	int w, h;
	glfwGetFramebufferSize(window, &w, &h); // Note, this will be 2x the resolution given to glfwCreateWindow on retina displays
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, w, h);
	glDepthRange(0, 10);
	glClearColor(1.f, 0, 0, 1.0);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	gl_has_errors();
	// Enabling alpha channel for textures
	glDisable(GL_BLEND);
	// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

	// Draw the screen texture on the quad geometry
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[(GLuint)GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE]);
	glBindBuffer(
		GL_ELEMENT_ARRAY_BUFFER,
		index_buffers[(GLuint)GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE]); // Note, GL_ELEMENT_ARRAY_BUFFER associates
																	 // indices to the bound GL_ARRAY_BUFFER
	gl_has_errors();
	const GLuint screen_program = effects[(GLuint)EFFECT_ASSET_ID::SCREEN];
	// Set clock
	GLuint fade_in_uloc = glGetUniformLocation(screen_program, "fade_in_factor");
	GLuint darken_uloc = glGetUniformLocation(screen_program, "darken_screen_factor");

	ScreenState& screen = registry.screenStates.get(screen_state_entity);
	glUniform1f(fade_in_uloc, screen.fade_in_factor);      // Use fade_in_factor for fade-in effect
	glUniform1f(darken_uloc, screen.darken_screen_factor); // Use darken_screen_factor for death effect
	gl_has_errors();

	// Set the vertex position and vertex texture coordinates (both stored in the
	// same VBO)
	GLint in_position_loc = glGetAttribLocation(screen_program, "in_position");
	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);
	gl_has_errors();

	// Bind our texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);

	glBindTexture(GL_TEXTURE_2D, off_screen_render_buffer_color);
	gl_has_errors();
	// Draw
	glDrawElements(
		GL_TRIANGLES, 3, GL_UNSIGNED_SHORT,
		nullptr); // one triangle = 3 vertices; nullptr indicates that there is
				  // no offset from the bound index buffer
	gl_has_errors();
}

// Render our game world
// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-14-render-to-texture/
void RenderSystem::draw()
{
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

	for (Entity entity : registry.tiles.entities) {
		if (!registry.motions.has(entity)) continue;
		drawTexturedMesh(entity, projection_2D);
	}
	// tile atlas works, but cant spawn player at the same time... will make separate function for player and mesh
	// robots and players not spawning
	for (Entity entity : registry.robots.entities) {
		if (!registry.motions.has(entity)) continue;
		drawTexturedMesh(entity, projection_2D);
	}
	if (registry.players.has(player)) {
		drawTexturedMesh(player, projection_2D);
		
	}

	for (Entity entity : registry.keys.entities) {
		if (!registry.motions.has(entity)) continue;
		drawTexturedMesh(entity, projection_2D);
	}

	drawHealthBar(player, ui_projection);

	// Truely render to the screen
	drawToScreen();

	// flicker-free display with a double buffer
	glfwSwapBuffers(window);
	gl_has_errors();
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

void RenderSystem::drawHealthBar(Entity player, const mat3& projection)
{
	if (!registry.players.has(player))
		return;

	// Get player health values
	Player& player_data = registry.players.get(player);
	Inventory& player_inventory = player_data.inventory;
	const auto& items = player_inventory.getItems();

	// Health percentage (between 0 and 1)
	float health_percentage = player_data.current_health / player_data.max_health;

	// Health bar position and size
	vec2 bar_position = vec2(20.f, window_height_px - 60.f); 
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

	// Use the correct shader for textured avatars
	glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED]);
	gl_has_errors();

	// Bind the VBO for avatar rendering 
	glBindBuffer(GL_ARRAY_BUFFER, healthbar_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(avatar_vertices), avatar_vertices, GL_DYNAMIC_DRAW);
	gl_has_errors();

	// Set up vertex attributes for textured drawing
	GLint in_position_loc = glGetAttribLocation(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED], "in_position");
	GLint in_texcoord_loc = glGetAttribLocation(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED], "in_texcoord");

	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
	glEnableVertexAttribArray(in_texcoord_loc);
	glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)sizeof(vec3));
	gl_has_errors();

	// Activate the texture for the avatar
	glActiveTexture(GL_TEXTURE0);
	GLuint avatar_texture_id = texture_gl_handles[(GLuint)TEXTURE_ASSET_ID::AVATAR]; // Assuming avatar texture is loaded
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
	vec3 health_color = glm::mix(vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), health_percentage);
	glBufferData(GL_ARRAY_BUFFER, sizeof(current_bar_vertices), current_bar_vertices, GL_DYNAMIC_DRAW);
	gl_has_errors();

	// Set the interpolated color for the current health bar
	glUniform3fv(color_uloc, 1, (float*)&health_color);
	gl_has_errors();

	// Draw the current health bar
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	gl_has_errors();

	// Draw the "Health" label below the health bar
	float text_scale = 0.5f;
	glm::vec3 font_color = glm::vec3(1.0f, 1.0f, 1.0f); // White color
	glm::mat4 font_trans = glm::mat4(1.0f); // Identity matrix

	// Calculate bottom-left position for the text
	float text_x = bar_position.x;
	float text_y = 20.0f;
	float health_percentage_text = (static_cast<float>(player_data.current_health) / player_data.max_health) * 100.0f;

	float health_text_x = bar_position.x;
	float health_text_y = bar_position.y + (bar_size.y / 2.0f) - (10.0f * text_scale);

	// Load the font and render the text
	std::string font_filename = PROJECT_SOURCE_DIR + std::string("data/fonts/PressStart2P.ttf");
	unsigned int font_default_size = 20;
	initializeFont(font_filename, font_default_size);

	renderText("Health", health_text_x, text_y, text_scale, font_color, font_trans);
	std::string percentage_text = std::to_string((int)health_percentage_text) + "%";
	float percentage_text_x = bar_position.x + bar_size.x - (40.0f); // for right alignment
	renderText(percentage_text, percentage_text_x, text_y, text_scale, font_color, font_trans);

	// Inventory Slots
	vec2 slot_size = vec2(190.f, 110.f);  
	float total_slots_width = (3 * slot_size.x)/1.5;  // 3 slots
	vec2 slot_position = vec2((window_width_px - total_slots_width) / 2, bar_position.y - slot_size.y + 10.f); // Centered horizontally and positioned above health bar

	// Draw three inventory slots centered on the screen
	glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::TEXTURED]);
	gl_has_errors();

	for (int i = 0; i < 3; ++i) {
		vec2 current_slot_position = slot_position + vec2(i * (slot_size.x)/1.5, 0.f);

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

		// Check if the current slot is the selected slot and set the texture accordingly
		GLuint slot_texture_id = (i == player_inventory.getSelectedSlot())
			? texture_gl_handles[(GLuint)TEXTURE_ASSET_ID::INVENTORY_SLOT_SELECTED]
			: texture_gl_handles[(GLuint)TEXTURE_ASSET_ID::INVENTORY_SLOT];

		glBindTexture(GL_TEXTURE_2D, slot_texture_id);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		gl_has_errors();

		// Draw item in the slot if present
		if (i < items.size()) {
			TEXTURE_ASSET_ID item_texture_enum = getTextureIDFromItemName(items[i].name);
			GLuint item_texture_id = texture_gl_handles[(GLuint)item_texture_enum];

			// Retrieve original item texture dimensions
			ivec2 original_size = texture_dimensions[(GLuint)item_texture_enum];

			// Calculate the scaling factor to fit the item within the slot while maintaining aspect ratio
			float scale_factor = std::min(slot_size.x / original_size.x, slot_size.y / original_size.y);
			vec2 item_size = vec2(original_size.x, original_size.y) * scale_factor * 0.5f;

			vec2 item_position = current_slot_position + (slot_size - item_size) / 2.0f; // Center item

			TexturedVertex item_vertices[4] = {
				{ vec3(item_position.x, item_position.y, 0.f), vec2(0.f, 1.f) },
				{ vec3(item_position.x + item_size.x, item_position.y, 0.f), vec2(1.f, 1.f) },
				{ vec3(item_position.x + item_size.x, item_position.y + item_size.y, 0.f), vec2(1.f, 0.f) },
				{ vec3(item_position.x, item_position.y + item_size.y, 0.f), vec2(0.f, 0.f) }
			};

			glBufferData(GL_ARRAY_BUFFER, sizeof(item_vertices), item_vertices, GL_DYNAMIC_DRAW);
			glBindTexture(GL_TEXTURE_2D, item_texture_id);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
			gl_has_errors();
		}
	}
}
TEXTURE_ASSET_ID RenderSystem::getTextureIDFromItemName(const std::string& itemName) {
	if (itemName == "Key") return TEXTURE_ASSET_ID::KEY;
	return TEXTURE_ASSET_ID::KEY;// default (should replace with empty)
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

std::map<char, Character> Characters;
FT_Library ft;
FT_Face face;
std::string readShaderFile(const std::string& filename)
{
	//std::cout << "Loading shader filename: " << filename << std::endl;

	std::ifstream ifs(filename);

	if (!ifs.good())
	{
		std::cerr << "ERROR: invalid filename loading shader from file: " << filename << std::endl;
		return "";
	}

	std::ostringstream oss;
	oss << ifs.rdbuf();
	//std::cout << oss.str() << std::endl;
	return oss.str();
}
bool RenderSystem::initializeFont(const std::string& font_path, unsigned int font_size) {

	// apply orthographic projection matrix for font, i.e., screen space
	/*fontShaderProgram = effects[(GLuint)EFFECT_ASSET_ID::FONT];*/
		// read in our shader files
	std::string vertexShaderSource = readShaderFile(PROJECT_SOURCE_DIR + std::string("shaders/font.vs.glsl"));
	std::string fragmentShaderSource = readShaderFile(PROJECT_SOURCE_DIR + std::string("shaders/font.fs.glsl"));
	const char* vertexShaderSource_c = vertexShaderSource.c_str();
	const char* fragmentShaderSource_c = fragmentShaderSource.c_str();

	// enable blending or you will just get solid boxes instead of text
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// font buffer setup
	glGenVertexArrays(1, &text_vao);
	glGenBuffers(1, &text_vbo);

	// font vertex shader
	unsigned int font_vertexShader;
	font_vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(font_vertexShader, 1, &vertexShaderSource_c, NULL);
	glCompileShader(font_vertexShader);

	// font fragement shader
	unsigned int font_fragmentShader;
	font_fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(font_fragmentShader, 1, &fragmentShaderSource_c, NULL);
	glCompileShader(font_fragmentShader);

	// font shader program
	fontShaderProgram = glCreateProgram();
	glAttachShader(fontShaderProgram, font_vertexShader);
	glAttachShader(fontShaderProgram, font_fragmentShader);
	glLinkProgram(fontShaderProgram);


	glUseProgram(fontShaderProgram);
	glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(window_width_px), 0.0f, static_cast<float>(window_height_px));
	GLint project_location = glGetUniformLocation(fontShaderProgram, "projection");
	assert(project_location > -1);
	//std::cout << "project_location: " << project_location << std::endl;
	glUniformMatrix4fv(project_location, 1, GL_FALSE, glm::value_ptr(projection));

	// init FreeType fonts
	FT_Library ft;
	if (FT_Init_FreeType(&ft))
	{
		std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
		return false;
	}

	FT_Face face;
	if (FT_New_Face(ft, font_path.c_str(), 0, &face))
	{
		std::cerr << "ERROR::FREETYPE: Failed to load font: " << font_path << std::endl;
		return false;
	}

	// extract a default size
	FT_Set_Pixel_Sizes(face, 0, font_size);

	// disable byte-alignment restriction in OpenGL
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// load each of the chars - note only first 128 ASCII chars
	for (unsigned char c = (unsigned char)0; c < (unsigned char)128; c++)
	{
		// load character glyph 
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
		{
			std::cerr << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
			continue;
		}

		// generate texture
		unsigned int texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);

		//std::cout << "texture: " << c << " = " << texture << std::endl;

		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer
		);

		// set texture options
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// now store character for later use
		Character character = {
			texture,
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			static_cast<unsigned int>(face->glyph->advance.x),
			(char)c
		};
		Characters.insert(std::pair<char, Character>(c, character));
	}
	glBindTexture(GL_TEXTURE_2D, 0);

	// clean up
	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	// bind buffers
	glBindVertexArray(text_vao);
	glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

	//// release buffers
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	//glBindVertexArray(0);

	return true;
}
void RenderSystem::renderText(std::string text, float x, float y, float scale, const glm::vec3& color, const glm::mat4& trans) {
	// Activate corresponding render state

	glUseProgram(fontShaderProgram);
	gl_has_errors();
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
