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
		textures_path("boss_robot.png"),
		textures_path("player_idle.png"),
		textures_path("player_fullsheet.png"),
		textures_path("crockbot_fullsheet.png"),
		textures_path("boss_fullsheet.png"),
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
		textures_path("Key.png"),
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
		textures_path("teleporter.png"),
		textures_path("game_over.png"),
		textures_path("start_screen.png"),
		textures_path("armor_icon.png"),
		textures_path("weapon_icon.png"),
		textures_path("ice_robot_fullsheet.png"),
		textures_path("companion_ice_robot.png"),
		textures_path("companion_ice_robot_fullsheet.png"),
		textures_path("spaceship.png"),
		textures_path("bat.png"),
		textures_path("paused_ui.png"),
		textures_path("radiation_icon.png"),
		textures_path("cutscenes/C1.png"),
textures_path("cutscenes/C2.png"),
textures_path("cutscenes/C3.png"),
textures_path("cutscenes/C4.png"),
textures_path("cutscenes/C5.png"),
textures_path("cutscenes/C6.png"),
textures_path("cutscenes/C7.png"),
textures_path("cutscenes/C8.png"),
textures_path("cutscenes/C9.png"),
textures_path("cutscenes/C10.png"),
textures_path("cutscenes/C11.png"),
textures_path("cutscenes/C12.png"),
textures_path("cutscenes/C13.png"),
textures_path("cutscenes/C14.png"),
textures_path("cutscenes/C15.png"),
textures_path("cutscenes/C16.png"),
textures_path("cutscenes/C17.png"),
textures_path("cutscenes/C18.png"),
textures_path("cutscenes/C19.png"),
textures_path("cutscenes/C20.png"),
textures_path("cutscenes/C21.png"),
textures_path("cutscenes/C22.png"),
textures_path("cutscenes/C23.png"),
textures_path("cutscenes/C24.png"),
textures_path("cutscenes/C25.png"),
textures_path("cutscenes/C26.png"),
textures_path("cutscenes/C27.png"),
textures_path("cutscenes/C28.png"),
textures_path("cutscenes/C29.png"),
textures_path("cutscenes/C30.png"),
textures_path("cutscenes/C31.png"),
textures_path("cutscenes/C32.png"),
textures_path("cutscenes/C33.png"),
textures_path("cutscenes/C34.png"),
textures_path("cutscenes/C35.png"),
textures_path("cutscenes/C36.png"),
textures_path("cutscenes/C37.png"),
textures_path("cutscenes/C38.png"),
textures_path("cutscenes/C39.png"),
textures_path("cutscenes/C40.png"),
textures_path("cutscenes/C41.png"),
textures_path("cutscenes/C42.png"),
textures_path("cutscenes/C43.png"),
textures_path("cutscenes/C44.png"),
textures_path("cutscenes/C45.png"),
textures_path("cutscenes/C46.png"),
textures_path("cutscenes/C47.png"),
textures_path("cutscenes/C48.png"),
textures_path("cutscenes/C49.png"),
textures_path("cutscenes/C50.png"),
textures_path("cutscenes/C51.png"),
textures_path("cutscenes/C52.png"),
textures_path("cutscenes/C53.png"),
textures_path("cutscenes/C54.png"),
textures_path("cutscenes/C55.png"),
textures_path("cutscenes/C56.png"),
textures_path("cutscenes/C57.png"),
textures_path("cutscenes/C58.png"),
textures_path("cutscenes/C59.png"),
textures_path("cutscenes/C60.png"),
textures_path("cutscenes/C61.png"),
textures_path("cutscenes/C62.png"),
textures_path("cutscenes/C63.png"),
textures_path("cutscenes/C64.png"),
textures_path("cutscenes/C65.png"),
textures_path("cutscenes/C66.png"),
textures_path("cutscenes/C67.png"),
textures_path("cutscenes/C68.png"),
textures_path("cutscenes/C69.png"),
textures_path("cutscenes/C70.png"),
textures_path("cutscenes/C71.png"),
textures_path("cutscenes/C72.png"),
textures_path("cutscenes/C73.png"),
textures_path("cutscenes/C74.png"),
textures_path("cutscenes/C75.png"),
textures_path("cutscenes/C76.png"),
textures_path("cutscenes/C77.png"),
textures_path("cutscenes/C78.png"),
textures_path("cutscenes/C79.png"),
textures_path("cutscenes/C80.png"),
textures_path("cutscenes/C81.png"),
textures_path("cutscenes/C82.png"),
textures_path("cutscenes/C83.png"),
textures_path("cutscenes/C84.png"),
textures_path("cutscenes/C85.png"),
textures_path("cutscenes/C86.png"),
textures_path("cutscenes/C87.png"),
textures_path("cutscenes/C88.png"),
textures_path("cutscenes/C89.png"),
textures_path("cutscenes/C90.png"),
textures_path("cutscenes/C91.png"),
textures_path("cutscenes/C92.png"),
textures_path("cutscenes/C93.png"),
textures_path("cutscenes/C94.png"),
textures_path("cutscenes/C95.png"),
textures_path("cutscenes/C96.png"),
textures_path("cutscenes/C97.png"),
textures_path("cutscenes/C98.png"),
textures_path("cutscenes/C99.png"),
textures_path("cutscenes/C100.png"),
textures_path("cutscenes/C101.png"),
textures_path("cutscenes/C102.png"),
textures_path("cutscenes/C103.png"),
textures_path("cutscenes/C104.png"),
textures_path("cutscenes/C105.png"),
		textures_path("spiderrobot_fullsheet.png")
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
	std::string pickup_item_name;
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
	void RenderSystem::drawSpaceshipTexture(Entity entity, const mat3& projection);
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
	float RenderSystem::getTextWidth(const std::string& text, float scale);
	TutorialState tutorial_state;
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
	bool game_paused = false;
	int hovered_menu_index = -1;
	void RenderSystem::renderStartScreen();
	void RenderSystem::initStartScreenVBO();
	void RenderSystem::drawPausedUI(const mat3& projection);

	bool show_game_over_screen = false;
	void RenderSystem::renderGameOverScreen();

	// Cutscene state
	bool playing_cutscene = false;                      
	std::vector<TEXTURE_ASSET_ID> cutscene_images;      
	size_t current_cutscene_index = 0;                  
	float cutscene_timer = 0.f;                         
	float cutscene_duration_per_image = 0.8f;           

	void startCutscene(const std::vector<TEXTURE_ASSET_ID>& images);
	//void updateCutscene(float elapsed_time);
	void renderCutscene();
	void skipCutscene();

	GLuint cutscene_vao = 0, cutscene_vbo = 0; 
	bool cutscene_vbo_initialized = false;
	void initCutsceneVBO();

	HelpOverlay helpOverlay;
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

	GLuint default_vao;

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


	std::string vertexShaderSource;
	std::string fragmentShaderSource;
	const char* vertexShaderSource_c;
	const char* fragmentShaderSource_c;

};

bool loadEffectFromFile(
	const std::string& vs_path, const std::string& fs_path, GLuint& out_program);
