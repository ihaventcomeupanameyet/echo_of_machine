#include "world_init.hpp"
#include "tiny_ecs_registry.hpp"
#include "tileset.hpp"

Entity createPlayer(RenderSystem* renderer, vec2 pos)
{
	auto entity = Entity();
	printf("CREATING PLAYER!\n");

	// Setting initial motion values
	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = vec2({ PLAYER_BB_WIDTH, PLAYER_BB_HEIGHT });
	//motion.scale.y *= -1; // point front to the right

	// create an empty component for our character
	Player& player = registry.players.emplace(entity);

	Inventory& inventory = player.inventory;
	// test items
	inventory.addItem("Robot Hand", 2);


	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::PLAYER_IDLE,    
		  EFFECT_ASSET_ID::TEXTURED,     
		  GEOMETRY_BUFFER_ID::SPRITE });    


	return entity;
}



Entity createRobot(RenderSystem* renderer, vec2 position)
{
	auto entity = Entity();
	//printf("Creating Robot\n");
	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Initialize the motion
	auto& motion = registry.motions.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.position = position;
	motion.scale.y *= -1;



	// Setting initial values, scale is negative to make it face the opposite way
	motion.scale = vec2({ -ROBOT_BB_WIDTH, ROBOT_BB_HEIGHT });

	// create an empty Robot component to be able to refer to all robots
	registry.robots.emplace(entity);
	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::ROBOT,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE
		});

	return entity;
}

Entity createKey(RenderSystem* renderer, vec2 position) {
	auto entity = Entity();
	//printf("CREATING Key!\n");

	// Setting initial motion values
	Motion& motion = registry.motions.emplace(entity);
	motion.position = position;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	// need to find the BB of the key
	motion.scale = vec2({ KEY_BB_WIDTH, KEY_BB_HEIGHT });
	//motion.scale.y *= -1; // point front to the right

	// create an empty component for the key
	registry.keys.emplace(entity);


	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::KEY,
		  EFFECT_ASSET_ID::TEXTURED,
		  GEOMETRY_BUFFER_ID::SPRITE });


	return entity;
}

Entity createTileEntity(RenderSystem* renderer, TileSet& tileset, vec2 position, float tile_size, int tile_id) {
	// Create a new entity for the tile
	Entity tile_entity = Entity();

	// Add motion component for positioning and scaling
	Motion& motion = registry.motions.emplace(tile_entity);
	motion.position = position;
	//motion.scale.x *= -1;
	//motion.scale.y *= -1;
	motion.scale = {tile_size, -tile_size };

	// Add the tile component
	Tile& tile = registry.tiles.emplace(tile_entity);
	tile.tile_id = tile_id;

		registry.renderRequests.insert(
			tile_entity,
			{ TEXTURE_ASSET_ID::TILE_ATLAS, EFFECT_ASSET_ID::TEXTURED, GEOMETRY_BUFFER_ID::SPRITE }
		);

	return tile_entity;
}

Entity createLine(vec2 position, vec2 scale)
{
	Entity entity = Entity();

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	//registry.renderRequests.insert(
	//	entity, {
	//		TEXTURE_ASSET_ID::TEXTURE_COUNT,
	//		EFFECT_ASSET_ID::EGG,
	//		GEOMETRY_BUFFER_ID::DEBUG_LINE
	//	});

	// Create motion
	Motion& motion = registry.motions.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0, 0 };
	motion.position = position;
	motion.scale = scale;

	registry.debugComponents.emplace(entity);
	return entity;
}

Entity createTile_map(std::vector<std::vector<int>> tile_map, int tile_size) {
	T_map t;
	t.tile_map = tile_map;
	t.tile_size = tile_size;
	Entity T_ent = Entity();
	registry.maps.insert(T_ent, t);
	return T_ent;
}
