#pragma once

#include <array>
#include <utility>

#include <SDL.h>

#include "common.hpp"
#include "components.hpp"
#include "tiny_ecs.hpp"
#include "tileset.hpp"
#include <map>
#include "help_overlay.hpp"
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
	

	// FPS counter variables
	Uint32 last_time = 0;        // time of the last FPS update
	Uint32 frame_count = 0;      // frame count in the current second
	float fps = 0.0f;            // calculated FPS value

	// Make sure these paths remain in sync with the associated enumerators.
	// Associated id with .obj path
	const std::vector < std::pair<GEOMETRY_BUFFER_ID, std::string>> mesh_paths =
	{
		 std::pair<GEOMETRY_BUFFER_ID, std::string>(GEOMETRY_BUFFER_ID::SPACESHIP, mesh_path("spaceship.obj"))
		  // specify meshes of other assets here
	};

	// Make sure these paths remain in sync with the associated enumerators.
	const std::array<std::string, texture_count> texture_paths = {
		textures_path("robot.png"),
		textures_path("boss_fullsheet.png"),
		textures_path("player_idle.png"),
		textures_path("player_fullsheet.png"),
		textures_path("crockbot_fullsheet.png"),
		textures_path("rightdoorsheet.png"),
		textures_path("bottomdoorsheet.png"),
		textures_path("healthpotion.png"),
		textures_path("tile_atlas.png"),
		textures_path("tile_atlas_levels.png"),
		textures_path("avatar.png"),
		textures_path("inventory_slot.png"),
		textures_path("inventory_slot_selected.png"),
		textures_path("ui_screen.png"),
		textures_path("player_upgrade_slot.png"),
		textures_path("Key 1.png"),
		textures_path("ArmorPlate.png"),
		textures_path("weapon_slot.png"),
		textures_path("armor_slot.png"),
		textures_path("upgrade_button.png"),
		textures_path("upgrade_button_hover.png"),
		textures_path("player_avatar.png"),
		textures_path("inv_slot.png"),
		textures_path("capture_ui.png"),
		textures_path("c_button.png"),
		textures_path("c_button_hover.png"),
		textures_path("d_button.png"),
		textures_path("d_button_hover.png"),
		textures_path("projectile.png"),
		textures_path("ice_proj.png"),
		textures_path("smoke.png"),
		textures_path("companion_crockbot.png"),
		textures_path("companion_crockbot_fullsheet.png"),
		textures_path("robot_part.png"),
		textures_path("energy_core.png"),
		textures_path("speed_booster.png"),
		textures_path("start_screen.png"),
		textures_path("armor_icon.png"),
		textures_path("weapon_icon.png")
	};

	//const std::array<std::string, texture_count> tile_atlas_paths = {
	//	textures_path("tile_atlas.png"),
	//};


public:
	GLuint getVertexBuffer(GEOMETRY_BUFFER_ID id) const {
		return vertex_buffers[(GLuint)id];
	}

	void updateCameraPosition(vec2 player_position);
	void RenderSystem::drawHUD(Entity player, const mat3& projection);
	void RenderSystem::initHealthBarVBO();
	void RenderSystem::initUIVBO();
	void RenderSystem::renderCaptureUI(const Robot& robot, Entity entity);
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
		shader_path("box"), // comment from ashish -> not sure but shouldn't here be font instead of having box twice??
		shader_path("spaceship")
	};



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
	void toggleHelp() { helpOverlay.toggle(); }

	void RenderSystem::drawReactionBox(Entity entity, const mat3& projection);
	void RenderSystem::drawBossReactionBox(Entity entity, const mat3& projection);
	void RenderSystem::renderButton(const vec2& position, const vec2& size, TEXTURE_ASSET_ID texture_id, TEXTURE_ASSET_ID hover_texture_id, const vec2& mouse_position);
	void RenderSystem::renderStatBar(const vec2& bar_position, const vec2& bar_size, float percentage, float stat_value);
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
	void RenderSystem::drawRobotHealthBar(Entity robot, const mat3& projection);
	void RenderSystem::initRobotHealthBarVBO();
	void RenderSystem::drawBossRobotHealthBar(Entity robot, const mat3& projection);
	void RenderSystem::initBossRobotHealthBarVBO();
	
// FPS functions
	bool show_fps = false;
	void updateFPS();
	void drawFPSCounter(const mat3& projection);
	std::vector<Item> droppedItems;
	GLuint fontShaderProgram;
	std::map<char, Character> Characters;
	FT_Library ft;
	FT_Face face;
	vec2 RenderSystem::getSlotPosition(int slot_index) const;
	bool isDragging = false;    // True if dragging an item
	int draggedSlot = -1;       // Index of the currently dragged slot
	glm::vec2 dragOffset;       // Offset for dragging to keep item centered
	glm::vec2 mousePosition;
	bool mouseReleased;
	bool isHelpVisible() const { return helpOverlay.isVisible(); }
	Entity currentRobotEntity;
	bool show_capture_ui = false;
	std::vector<Item> disassembledItems;
	bool isNighttime = false;
	bool key_spawned = false;

	// show start screen
	bool show_start_screen = true;

	void RenderSystem::renderStartScreen();
	void RenderSystem::initStartScreenVBO();

private:
	// Internal drawing functions for each entity type
	void drawTexturedMesh(Entity entity, const mat3& projection);
	void drawToScreen();
	HelpOverlay helpOverlay;
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
	GLuint startscreen_vbo;
	GLuint startscreen_vao;
	bool healthbar_vbo_initialized = false;
	bool font_initialized = false;
	bool tile_vbo_initialized = false;
	bool ui_vbo_initialized = false;
	bool startscreen_vbo_initialized = false;

	GLuint robot_healthbar_vbo;
	GLuint robot_healthbar_vao;
	bool robot_healthbar_vbo_initialized = false;
	GLuint boss_robot_healthbar_vbo;
	GLuint boss_robot_healthbar_vao;
	bool boss_robot_healthbar_vbo_initialized = false;
	std::string vertexShaderSource;
	std::string fragmentShaderSource;
	const char* vertexShaderSource_c;
	const char* fragmentShaderSource_c;

};

bool loadEffectFromFile(
	const std::string& vs_path, const std::string& fs_path, GLuint& out_program);
