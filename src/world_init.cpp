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
	animation = PlayerAnimation(64, 448, 1792);

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
	motion.scale = vec2({ ROBOT_BB_WIDTH, ROBOT_BB_HEIGHT }); 
	motion.bb = vec2(64, 64); 

	Robot& robot = registry.robots.emplace(entity);
	robot.search_box = { 15 * 64.f, 15 * 64.f };
	robot.attack_box = { 7.5 * 64.f, 7.5 * 64.f };
	robot.panic_box = { 4 * 64.f, 4 * 64.f };
	robot.current_health = companionRobotItem.health;
	robot.attack = companionRobotItem.damage;
	robot.speed = companionRobotItem.speed;

	robot.companion = true;

	auto& robotAnimation = registry.robotAnimations.emplace(entity);
	robotAnimation = RobotAnimation(64, 640, 1280);
	robotAnimation.setState(RobotState::IDLE, Direction::LEFT);

	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::COMPANION_CROCKBOT_FULLSHEET, 
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE
		});

	return entity;
}

Entity createCompanionIceRobot(RenderSystem* renderer, vec2 position, const Item& companionRobotItem) {
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
	r.attack_box = { 7.5 * 64.f,7.5 * 64.f };
	r.panic_box = { 0 * 64.f,0 * 64.f };
	r.ice_proj = true;
	r.companion = true;
	auto& robotAnimation = registry.iceRobotAnimations.emplace(entity);
	robotAnimation = IceRobotAnimation(64, 832, 1024);
	robotAnimation.setState(IceRobotState::IDLE, Direction::LEFT);
	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::COMPANION_ICE_ROBOT_FULLSHEET,
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
	r.attack_box = { 7.5 * 64.f,7.5 * 64.f };
	r.panic_box = { 0 * 64.f,0 * 64.f };

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


Entity createBossRobot(RenderSystem* renderer, vec2 position)
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
	motion.scale = vec2({ BOSS_ROBOT_BB_WIDTH, BOSS_ROBOT_BB_HEIGHT });

	motion.bb = vec2(64, 64);
	// create an empty Robot component to be able to refer to all robots
	BossRobot& r = registry.bossRobots.emplace(entity);
	r.search_box = { 30 * 64.f,30 * 64.f };
	r.attack_box = { 7.5 * 64.f,7.5 * 64.f };
	r.panic_box = { 4 * 64.f,4 * 64.f };

	auto& bossRobotAnimation = registry.bossRobotAnimations.emplace(entity);
	bossRobotAnimation = BossRobotAnimation(128, 1792, 384);
	bossRobotAnimation.setState(BossRobotState::IDLE, Direction::LEFT);
	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::BOSS_FULLSHEET,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE
		});

	return entity;
}
Entity createSpiderRobot(RenderSystem* renderer, vec2 position)
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
	motion.scale = vec2({ ROBOT_BB_WIDTH*0.3f, ROBOT_BB_HEIGHT * 0.3f });

	motion.bb = vec2(64, 64);
	// create an empty Robot component to be able to refer to all robots
	SpiderRobot& r = registry.spiderRobots.emplace(entity);
	r.search_box = { 6 * 64.f,6 * 64.f };
	r.attack_box = { 1 * 64.f,1 * 64.f };
	r.panic_box = { 0 * 64.f,0 * 64.f };

	auto& bossRobotAnimation = registry.spiderRobotAnimations.emplace(entity);
	bossRobotAnimation = SpiderRobotAnimation(16, 64, 64);
	bossRobotAnimation.setState(SpiderRobotState::IDLE, Direction::LEFT);
	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::SPIDERROBOT_FULLSHEET,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE
		});

	return entity;
}
Entity createIceRobot(RenderSystem* renderer, vec2 position)
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
	r.attack_box = { 7.5 * 64.f,7.5 * 64.f };
	r.panic_box = { 0 * 64.f,0 * 64.f };
	r.ice_proj = true;
	auto& robotAnimation = registry.iceRobotAnimations.emplace(entity);
	robotAnimation = IceRobotAnimation(64, 832, 1024);
	robotAnimation.setState(IceRobotState::IDLE, Direction::LEFT);
	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::ICE_ROBOT_FULLSHEET,
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

	motion.scale = vec2(mesh.original_size.x * 7.5f, -mesh.original_size.y * 7.5f);
	motion.bb = motion.scale;

	registry.spaceships.emplace(entity);
	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::SPACESHIP,
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
Entity createNotification(const std::string& text, float duration, vec2 position, vec3 color, float scale) {
	auto entity = Entity();

	if (position.x == -1) {
		float screenWidth = window_width_px; 
		position.x = screenWidth / 2.0f;
	}

	registry.notifications.emplace(entity, Notification(text, duration, position, color, scale));
	return entity;
}





Entity createProjectile(vec2 position,vec2 speed,float angle,bool ice, bool player_projectile) {
	auto entity = Entity();


	Motion& motion = registry.motions.emplace(entity);
	motion.position = position;
	motion.angle = angle;
	motion.velocity = speed;
	motion.target_velocity = speed;
	// need to find the BB of the key
	motion.scale = vec2({ 127, 123 });
	//motion.scale.y *= -1; // point front to the right

	// create an empty component for the projectile
	projectile& temp = registry.projectile.emplace(entity);
	if (player_projectile) {
		temp.dmg = ice ? 5 : 10;
		temp.friendly = true; 
	}
	else {
		temp.dmg = ice ? 5 : 10; 
		temp.friendly = false;
	}
	temp.ice = ice;
	motion.bb = {32.f,32.f};


	if (player_projectile) {
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
	}
	else {
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
	}


	return entity;
}

Entity createBossProjectile(vec2 position,vec2 speed,float angle,int dmg) {
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
	bossProjectile& temp = registry.bossProjectile.emplace(entity);
	temp.dmg = dmg;
	temp.amplitude = 2.5f;
	temp.frequency = 4.0f;
	temp.time = 0.0f;

	motion.bb = {32.f,32.f};



	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::PROJECTILE,
		  EFFECT_ASSET_ID::TEXTURED,
		  GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}


Entity createRightDoor(RenderSystem* renderer, vec2 position) {
	auto entity = Entity();
	printf("CREATING RIGHT DOOR!\n");

	Motion& motion = registry.motions.emplace(entity);
	motion.position = position;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = vec2({ 256, 256 });
	motion.bb = vec2(1, 256);

	auto& animation = registry.doorAnimations.emplace(entity);
	animation = DoorAnimation(128, 768, 128);
	animation.current_frame = 0;
	animation.is_opening = false;

	Door& door = registry.doors.emplace(entity);
	door.is_right_door = true;
	door.is_locked = true;

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::RIGHTDOORSHEET,
		  EFFECT_ASSET_ID::TEXTURED,
		  GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}

Entity createBottomDoor(RenderSystem* renderer, vec2 position) {
	auto entity = Entity();
	printf("CREATING BOTTOM DOOR!\n");

	Motion& motion = registry.motions.emplace(entity);
	motion.position = position;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = vec2({ 256, 256 });
	motion.bb = vec2(256, 1);

	auto& animation = registry.doorAnimations.emplace(entity);
	animation = DoorAnimation(128, 768, 128);
	animation.current_frame = 0;
	animation.is_opening = false;

	Door& door = registry.doors.emplace(entity);
	door.is_right_door = false;
	door.is_locked = true;

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::BOTTOMDOORSHEET,
		  EFFECT_ASSET_ID::TEXTURED,
		  GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}


Entity createSmokeParticle(RenderSystem* renderer, vec2 position)
{
	auto entity = Entity();
	Motion& motion = registry.motions.emplace(entity);
	motion.position = position;

	float base_size = 20.0f + static_cast<float>(rand()) / RAND_MAX * 15.0f;
	motion.scale = vec2(base_size);

	float random_x = -30.0f + static_cast<float>(rand()) / RAND_MAX * 60.0f;
	float random_y = -15.0f + static_cast<float>(rand()) / RAND_MAX * 10.0f;
	motion.velocity = { random_x, random_y };
	motion.bb = motion.scale;

	Particle& particle = registry.particles.emplace(entity);
	particle.lifetime = 0.f;
	particle.max_lifetime = 2.0f + static_cast<float>(rand()) / RAND_MAX * 1.0f;
	particle.opacity = 0.3f + static_cast<float>(rand()) / RAND_MAX * 0.2f;
	particle.size = motion.scale.x;

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::SMOKE,
		  EFFECT_ASSET_ID::TEXTURED,
		  GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}

Entity createBat(RenderSystem* renderer, vec2 position) {
	auto entity = Entity();

	Motion& motion = registry.motions.emplace(entity);
	motion.position = position;
	motion.angle = 0.f;

	float angle = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2 * M_PI;
	motion.velocity = vec2(cos(angle), sin(angle)) * 80.f;
	motion.target_velocity = motion.velocity;
	motion.scale = vec2(32.f, 32.f);
	motion.bb = motion.scale;

	Boid& boid = registry.boids.emplace(entity);
	boid.separation_weight = 1.5f;   
	boid.alignment_weight = 1.2;    
	boid.cohesion_weight = 1.2f;   
	boid.chase_weight = 1.0f;  

	boid.max_speed = 180.f;  
	boid.max_force = 15.f;

	boid.avoid_radius = vec2(35.f);   
	boid.search_radius = vec2(120.f);
	boid.attack_radius = vec2(125.f);

	boid.damage = 1;

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::BAT,
		  EFFECT_ASSET_ID::TEXTURED,
		  GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}