// internal
#include "world_init.hpp"
#include "tiny_ecs_registry.hpp"

Entity createPlayer(RenderSystem* renderer, vec2 pos)
{
	auto entity = Entity();

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
	inventory.addItem("Key", 1);


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
