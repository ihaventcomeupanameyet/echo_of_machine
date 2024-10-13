// internal
#include "render_system.hpp"
#include <SDL.h>
#include "tileset.hpp"

#include "tiny_ecs_registry.hpp"


void RenderSystem::drawTexturedMesh(Entity entity, const mat3& projection)
{
	Motion& motion = registry.motions.get(entity);
	Transform transform;
	transform.translate(motion.position);
	transform.rotate(motion.angle);
	transform.scale(motion.scale);
	// correctly scale at 1.0
	transform.scale(vec2(1.0, 1.0));

	assert(registry.renderRequests.has(entity));
	const RenderRequest& render_request = registry.renderRequests.get(entity);

	const GLuint used_effect_enum = (GLuint)render_request.used_effect;
	assert(used_effect_enum != (GLuint)EFFECT_ASSET_ID::EFFECT_COUNT);
	const GLuint program = (GLuint)effects[used_effect_enum];

	// Setting shaders
	glUseProgram(program);
	gl_has_errors();

	// check if the texture is the tile atlas
	if (render_request.used_texture == TEXTURE_ASSET_ID::TILE_ATLAS) {
		// get tile component associated with the entity to retrieve the tile ID
		assert(registry.tiles.has(entity));
		const Tile& tile = registry.tiles.get(entity);

		// get the TileSetComponent
		const TileSetComponent& tileset_component = registry.tilesets.get(registry.tilesets.entities[0]);

		// Get the tile data (coordinates in the atlas) based on the tile_id
		const TileData& tile_data = tileset_component.tileset.getTileData(tile.tile_id);

		// create the vertices using the tile's texture coordinates
		TexturedVertex vertices[4] = {
			{ vec3(-0.5f, 0.5f, -0.1f), tile_data.top_left },   // top-left
			{ vec3(0.5f, 0.5f, -0.1f), vec2(tile_data.bottom_right.x, tile_data.top_left.y) },  // top-right
			{ vec3(0.5f, -0.5f, -0.1f), tile_data.bottom_right },  // bottom-right
			{ vec3(-0.5f, -0.5f, -0.1f), vec2(tile_data.top_left.x, tile_data.bottom_right.y) }  // bottm-left
		};

		// separate VBO for tiles
		if (!tile_vbo_initialized) {
			glGenBuffers(1, &tile_vbo);
			glGenBuffers(1, &tile_ibo);
			tile_vbo_initialized = true;
		}

		glBindBuffer(GL_ARRAY_BUFFER, tile_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
		gl_has_errors();

		// tile ibo
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tile_ibo);
		static const uint16_t tile_indices[] = { 0, 1, 2, 2, 3, 0 }; // two triangles forming a square
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(tile_indices), tile_indices, GL_DYNAMIC_DRAW);
		gl_has_errors();
	}
	else {
		// render rest of the entities
		const GLuint vbo = vertex_buffers[(GLuint)render_request.used_geometry];
		const GLuint ibo = index_buffers[(GLuint)render_request.used_geometry];

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		gl_has_errors();
	}

	// Input data location as in the vertex buffer
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

	glActiveTexture(GL_TEXTURE0);
	gl_has_errors();

	GLuint texture_id = texture_gl_handles[(GLuint)registry.renderRequests.get(entity).used_texture];
	glBindTexture(GL_TEXTURE_2D, texture_id);

	//if (render_request.used_texture == TEXTURE_ASSET_ID::TILE_ATLAS) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//	}
	gl_has_errors();

	// Setting uniform values and projection as before
	GLint color_uloc = glGetUniformLocation(program, "fcolor");
	const vec3 color = registry.colors.has(entity) ? registry.colors.get(entity) : vec3(1);
	glUniform3fv(color_uloc, 1, (float*)&color);
	gl_has_errors();

	GLint size = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	gl_has_errors();

	GLsizei num_indices = size / sizeof(uint16_t);

	GLint currProgram;
	glGetIntegerv(GL_CURRENT_PROGRAM, &currProgram);

	// Setting uniform values for the transformation and projection matrices
	GLuint transform_loc = glGetUniformLocation(currProgram, "transform");
	glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&transform.mat);
	GLuint projection_loc = glGetUniformLocation(currProgram, "projection");
	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection);
	gl_has_errors();

	// Drawing of num_indices/3 triangles specified in the index buffer
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