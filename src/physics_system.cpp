// internal
#include "physics_system.hpp"
#include "world_init.hpp"
#include "math_utils.hpp"

void bound_check(Motion& mo);

void dumb_ai(Motion& mo);
// Returns the local bounding coordinates scaled by the current size of the entity
vec2 get_bounding_box(const Motion& motion)
{
	// abs is to avoid negative scale due to the facing direction.
	return { abs(motion.scale.x), abs(motion.scale.y) };
}

// This is a SUPER APPROXIMATE check that puts a circle around the bounding boxes and sees
// if the center point of either object is inside the other's bounding-box-circle. You can
// surely implement a more accurate detection
bool collides(const Motion& motion1, const Motion& motion2)
{
	vec2 dp = motion1.position - motion2.position;
	float dist_squared = dot(dp,dp);
	const vec2 other_bonding_box = get_bounding_box(motion1) / 2.f;
	const float other_r_squared = dot(other_bonding_box, other_bonding_box);
	const vec2 my_bonding_box = get_bounding_box(motion2) / 2.f;
	const float my_r_squared = dot(my_bonding_box, my_bonding_box);
	const float r_squared = max(other_r_squared, my_r_squared);
	if (dist_squared < r_squared)
		return true;
	return false;
}

void PhysicsSystem::step(float elapsed_ms)
{
	// Move fish based on how much time has passed, this is to (partially) avoid
	// having entities move at different speed based on the machine.
	auto& motion_registry = registry.motions;

	// Entity player = registry.players.entities[0];
	// Motion& player_motion = motion_registry.get(player);

	ComponentContainer<Motion>& motion_container = registry.motions;

	for(uint i = 0; i< motion_registry.size(); i++)
	{
		
		Motion& motion = motion_registry.components[i];
		Entity entity = motion_registry.entities[i];
		float step_seconds = elapsed_ms / 1000.f;
		if (registry.robots.has(entity) ) {
			dumb_ai(motion);
		}
		motion.velocity.x = linear_inter(motion.target_velocity.x, motion.velocity.x, step_seconds * 100.0f);
		motion.velocity.y = linear_inter(motion.target_velocity.y, motion.velocity.y, step_seconds * 100.0f);
		vec2 pos = motion.position;
		motion.position += motion.velocity * step_seconds;

		// bound check and check if hit with non-walkable tile
		if (!registry.tiles.has(entity)) {
			bound_check(motion);
			//Motion& motion_i = motion_container.components[i];
			//Entity entity_i = motion_container.entities[i];

			// note starting j at i+1 to compare all (i,j) pairs only once (and to not compare with itself)
			for (uint j = 0; j < motion_container.components.size(); j++)
			{
				Motion& motion_j = motion_container.components[j];
				if (collides(motion, motion_j))
				{
					Entity entity_j = motion_container.entities[j];
					// Create a collisions event
					// We are abusing the ECS system a bit in that we potentially insert muliple collisions for the same entity

					if (registry.tiles.has(entity_j)) {
						if (!registry.tiles.get(entity_j).walkable) {
							std::cout <<"Current x: "<< motion.position.x << "Current y: " << motion.position.y << std::endl;
							std::cout << "Before x: " << pos.x << "Before y : " << pos.y << std::endl;
							motion.position = pos;
							motion.velocity = vec2(0);
						}
					}
				}
			}
			bound_check(motion);
		}
	}


	// Check for collisions between all moving entities
    
	for (uint i = 0; i<motion_container.components.size(); i++)
	{
		Motion& motion_i = motion_container.components[i];
		Entity entity_i = motion_container.entities[i];
		
		// note starting j at i+1 to compare all (i,j) pairs only once (and to not compare with itself)
		for(uint j = i+1; j<motion_container.components.size(); j++)
		{
			Motion& motion_j = motion_container.components[j];
			if (collides(motion_i, motion_j))
			{
				Entity entity_j = motion_container.entities[j];
				// Create a collisions event
				// We are abusing the ECS system a bit in that we potentially insert muliple collisions for the same entity

				registry.collisions.emplace_with_duplicates(entity_i, entity_j);
				registry.collisions.emplace_with_duplicates(entity_j, entity_i);
			}
		}
	}

}

void dumb_ai(Motion& mo) {
	Entity player = registry.players.entities[0];
	Motion& player_motion = registry.motions.get(player);
	mo.target_velocity = glm::normalize((player_motion.position - mo.position)) * vec2(50);
}

void bound_check(Motion& mo) {
	mo.position.x = max(min((float)window_width_px-16,mo.position.x),0.f+16);
	mo.position.y = max(min((float)window_height_px-32, mo.position.y), 0.f);
}