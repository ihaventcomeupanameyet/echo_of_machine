// internal
#include "render_system.hpp"
#include <SDL.h>
#include "tileset.hpp"

#include "tiny_ecs_registry.hpp"


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

	// Health percentage (between 0 and 1)
	float health_percentage = player_data.current_health / player_data.max_health;

	// Health bar position and size
	vec2 bar_position = vec2(20.f, window_height_px - 40.f); 
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
