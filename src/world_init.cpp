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

	auto& animation = registry.animations.emplace(entity);
	animation = PlayerAnimation(64, 448, 1280);

	motion.bb = vec2(64, 64);
	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::PLAYER_FULLSHEET,    
		  EFFECT_ASSET_ID::TEXTURED,     
		  GEOMETRY_BUFFER_ID::SPRITE });    


	return entity;
}

Entity createCompanionRobot(RenderSystem* renderer, vec2 position, const Item& companionRobotItem) {
	auto entity = Entity();

	// Mesh and Render setup
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Initialize motion component
	auto& motion = registry.motions.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.position = position;
	motion.scale = vec2({ ROBOT_BB_WIDTH, ROBOT_BB_HEIGHT }); // Set to predefined robot dimensions
	motion.bb = vec2(64, 64); // Bounding box for the robot

	// Create and set up Robot component using stats from the companion robot item
	Robot& robot = registry.robots.emplace(entity);
	robot.search_box = { 15 * 64.f, 15 * 64.f };
	robot.attack_box = { 10 * 64.f, 10 * 64.f };
	robot.panic_box = { 4 * 64.f, 4 * 64.f };
	robot.current_health = companionRobotItem.health;
	robot.attack = companionRobotItem.damage;
	robot.speed = companionRobotItem.speed;

	// Initialize the RobotAnimation component
	auto& robotAnimation = registry.robotAnimations.emplace(entity);
	robotAnimation = RobotAnimation(64, 640, 1280);
	robotAnimation.setState(RobotState::IDLE, Direction::LEFT);

	// Render request setup
	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::CROCKBOT_FULLSHEET, // Update this if your companion robot uses a different texture
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE
		});

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


	// Setting initial values, scale is negative to make it face the opposite way
	motion.scale = vec2({ ROBOT_BB_WIDTH, ROBOT_BB_HEIGHT });

	motion.bb = vec2(64, 64);
	// create an empty Robot component to be able to refer to all robots
	Robot& r = registry.robots.emplace(entity);
	r.search_box = { 15 * 64.f,15 * 64.f };
	r.attack_box = { 10 * 64.f,10 * 64.f };
	r.panic_box = { 4 * 64.f,4 * 64.f };

	auto& robotAnimation = registry.robotAnimations.emplace(entity);
	robotAnimation = RobotAnimation(64, 640, 1280);
	robotAnimation.setState(RobotState::IDLE, Direction::LEFT);
	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::CROCKBOT_FULLSHEET,
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

	motion.bb = motion.scale;

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::KEY,
		  EFFECT_ASSET_ID::TEXTURED,
		  GEOMETRY_BUFFER_ID::SPRITE });


	return entity;
}

Entity createPotion(RenderSystem* renderer, vec2 position) {
	auto entity = Entity();

	// Setting initial motion values
	Motion& motion = registry.motions.emplace(entity);
	motion.position = position;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	// need to find the BB of the key
	motion.scale = vec2({ POTION_BB_WIDTH, POTION_BB_HEIGHT });

	// create an empty component for the key
	registry.potions.emplace(entity);

	motion.bb = motion.scale;

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::HEALTHPOTION,
		  EFFECT_ASSET_ID::TEXTURED,
		  GEOMETRY_BUFFER_ID::SPRITE });


	return entity;
}

Entity createArmorPlate(RenderSystem* renderer, vec2 position) {
	auto entity = Entity();
	//printf("CREATING Key!\n");

	// Setting initial motion values
	Motion& motion = registry.motions.emplace(entity);
	motion.position = position;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	// need to find the BB of the key
	motion.scale = vec2({ ARMOR_BB_WIDTH, ARMOR_BB_HEIGHT });
	//motion.scale.y *= -1; // point front to the right

	// create an empty component for the key
	registry.armorplates.emplace(entity);

	motion.bb = motion.scale;

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::ARMORPLATE,
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

	motion.bb = motion.scale;


	// Add the tile component
	Tile& tile = registry.tiles.emplace(tile_entity);
	tile.tile_id = tile_id;

		registry.renderRequests.insert(
			tile_entity,
			{ TEXTURE_ASSET_ID::TILE_ATLAS, EFFECT_ASSET_ID::TEXTURED, GEOMETRY_BUFFER_ID::SPRITE }
		);

	return tile_entity;
}



Entity createSpaceship(RenderSystem* renderer, vec2 pos)
{
	auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPACESHIP);

	registry.meshPtrs.emplace(entity, &mesh);

	// Setting initial motion values
	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };

	motion.scale = -vec2(mesh.original_size.x * 3.5f, mesh.original_size.y * 3.5f);
	motion.bb = motion.scale;

	registry.spaceships.emplace(entity);
	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::TEXTURE_COUNT,
		  EFFECT_ASSET_ID::SPACESHIP,
		  GEOMETRY_BUFFER_ID::SPACESHIP });


	return entity;
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

attackBox initAB(vec2 pos, vec2 size, int dmg, bool friendly) {
	attackBox a;
	a.position = pos;
	a.bb = size;
	a.dmg = dmg;
	a.friendly = friendly;
	return a;
}



Entity createProjectile(vec2 position,vec2 speed,float angle,bool ice) {
	auto entity = Entity();


	Motion& motion = registry.motions.emplace(entity);
	motion.position = position;
	motion.angle = angle;
	motion.velocity = speed;
	motion.target_velocity = speed;
	// need to find the BB of the key
	motion.scale = vec2({ 127, 123 });
	//motion.scale.y *= -1; // point front to the right

	// create an empty component for the key
	projectile& temp = registry.projectile.emplace(entity);
	if (ice) {
		temp.dmg = 5;
	} else {
		temp.dmg = 10;
	}

	temp.ice = ice;
	motion.bb = {32.f,32.f};


	if (ice) {
		registry.renderRequests.insert(
			entity,
			{ TEXTURE_ASSET_ID::ICE_PROJ,
			  EFFECT_ASSET_ID::TEXTURED,
			  GEOMETRY_BUFFER_ID::SPRITE });
	}
	else {
		registry.renderRequests.insert(
			entity,
			{ TEXTURE_ASSET_ID::PROJECTILE,
			  EFFECT_ASSET_ID::TEXTURED,
			  GEOMETRY_BUFFER_ID::SPRITE });
	}


	return entity;
}