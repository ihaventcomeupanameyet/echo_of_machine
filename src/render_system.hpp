#pragma once

#include <array>
#include <utility>

#include "common.hpp"
#include "components.hpp"
#include "tiny_ecs.hpp"
#include "tileset.hpp"
#include <map>
// fonts
#include <ft2build.h>
#include FT_FREETYPE_H
#include <map>	
// System responsible for setting up OpenGL and for rendering all the
// visual entities in the game
class RenderSystem {
	/**
	 * The following arrays store the assets the game will use. They are loaded
	 * at initialization and are assumed to not be modified by the render loop.
	 *
	 * Whenever possible, add to these lists instead of creating dynamic state
	 * it is easier to debug and faster to execute for the computer.
	 */
	std::array<GLuint, texture_count> texture_gl_handles;
	std::array<ivec2, texture_count> texture_dimensions;

	vec2 camera_position;

	// Method to update the camera position
	

	// Make sure these paths remain in sync with the associated enumerators.
	// Associated id with .obj path
	const std::vector < std::pair<GEOMETRY_BUFFER_ID, std::string>> mesh_paths =
	{
		 // std::pair<GEOMETRY_BUFFER_ID, std::string>(GEOMETRY_BUFFER_ID::SALMON, mesh_path("salmon.obj"))
		  // specify meshes of other assets here
	};

	// Make sure these paths remain in sync with the associated enumerators.
	const std::array<std::string, texture_count> texture_paths = {
			textures_path("robot.png"),
			textures_path("player_idle.png"),
			textures_path("player_fullsheet.png"),
			// START OF TILE ATLAS
		/*	textures_path("tile_0.png"),
			textures_path("tile_1.png"),
			textures_path("tile_2.png"),
			textures_path("tile_3.png")*/
		textures_path("tile_atlas.png"),
		textures_path("tile_atlas_levels.png"),
		textures_path("avatar.png"),
		textures_path("inventory_slot.png"),
		textures_path("inventory_slot_selected.png"),
		textures_path("ui_screen.png"),
		textures_path("player_upgrade_slot.png"),
		textures_path("Key 1.png"),
		textures_path("ArmorPlate.png")
		
	};

	//const std::array<std::string, texture_count> tile_atlas_paths = {
	//	textures_path("tile_atlas.png"),
	//};


public:
	GLuint getVertexBuffer(GEOMETRY_BUFFER_ID id) const {
		return vertex_buffers[(GLuint)id];
	}

	void updateCameraPosition(vec2 player_position);
	void RenderSystem::drawHealthBar(Entity player, const mat3& projection);
	void RenderSystem::initHealthBarVBO();
	void RenderSystem::initUIVBO();

	Entity player;

	//GLuint tile_vbo;
	// TODO M1: Remove unecessary shaders for our game
	std::array<GLuint, effect_count> effects;
	// Make sure these paths remain in sync with the associated enumerators.
	const std::array<std::string, effect_count> effect_paths = {
		shader_path("coloured"),
		shader_path("textured"),
		shader_path("screen"),
		shader_path("box"),
		shader_path("box"), };



	std::array<GLuint, geometry_count> vertex_buffers;
	std::array<GLuint, geometry_count> index_buffers;
	/*std::array<GLuint, geometry_count> tile_vertex_buffers;
	std::array<GLuint, geometry_count> tile_index_buffers;*/
	std::array<Mesh, geometry_count> meshes;

public:
	// Initialize the window
	bool init(GLFWwindow* window);

	template <class T>
	void bindVBOandIBO(GEOMETRY_BUFFER_ID gid, std::vector<T> vertices, std::vector<uint16_t> indices);

	void initializeGlTextures();

	void initializeGlEffects();

	void initializeGlMeshes();
	Mesh& getMesh(GEOMETRY_BUFFER_ID id) { return meshes[(int)id]; };

	void RenderSystem::drawBoundingBox(Entity entity, const mat3& projection);

	void initializeGlGeometryBuffers();
	// Initialize the screen texture used as intermediate render target
	// The draw loop first renders to this texture, then it is used for the wind
	// shader
	bool initScreenTexture();

	// Destroy resources associated to one or all entities created by the system
	~RenderSystem();

	// Draw all entities
	void draw();

	mat3 createProjectionMatrix();
	mat3 createOrthographicProjection(float left, float right, float top, float bottom);
	void RenderSystem::drawInventoryUI();
	TEXTURE_ASSET_ID RenderSystem::getTextureIDFromItemName(const std::string& itemName);
	bool RenderSystem::initializeFont(const std::string& fontPath, unsigned int fontSize);
	void RenderSystem::renderText(std::string text, float x, float y, float scale, const glm::vec3& color, const glm::mat4& trans);
	void RenderSystem::renderInventoryItem(const Item& item, const vec2& position, const vec2& size);
	GLuint fontShaderProgram;
	std::map<char, Character> Characters;
	FT_Library ft;
	FT_Face face;
	vec2 RenderSystem::getSlotPosition(int slot_index) const;
	bool isDragging = false;    // True if dragging an item
	int draggedSlot = -1;       // Index of the currently dragged slot
	glm::vec2 dragOffset;       // Offset for dragging to keep item centered
	glm::vec2 mousePosition;
private:
	// Internal drawing functions for each entity type
	void drawTexturedMesh(Entity entity, const mat3& projection);
	void drawToScreen();
	// Window handle
	GLFWwindow* window;

	// Screen texture handles
	GLuint frame_buffer;
	GLuint off_screen_render_buffer_color;
	GLuint off_screen_render_buffer_depth;

	Entity screen_state_entity;

	GLuint tile_vbo = 0;   // VBO for tiles
	GLuint tile_ibo = 0;
	GLuint text_vao = 0; // Vertex Array Object for text rendering
	GLuint text_vbo = 0;
	GLuint ui_vbo;
	GLuint ui_vao;
	GLuint healthbar_vbo;
	GLuint healthbar_vao;
	bool healthbar_vbo_initialized = false;

	bool tile_vbo_initialized = false;
	bool ui_vbo_initialized = false;

};

bool loadEffectFromFile(
	const std::string& vs_path, const std::string& fs_path, GLuint& out_program);
