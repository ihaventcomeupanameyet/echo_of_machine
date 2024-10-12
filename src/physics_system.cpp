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
	float scale_factor = 0.35;

	vec2 size1 = get_bounding_box(motion1) * scale_factor;
	vec2 size2 = get_bounding_box(motion2) * scale_factor;

	vec2 pos1_min = motion1.position - (size1 / 2.f);
	vec2 pos1_max = motion1.position + (size1 / 2.f);

	vec2 pos2_min = motion2.position - (size2 / 2.f);
	vec2 pos2_max = motion2.position + (size2 / 2.f);

	bool overlap_x = (pos1_min.x <= pos2_max.x && pos1_max.x >= pos2_min.x);
	bool overlap_y = (pos1_min.y <= pos2_max.y - 10 && pos1_max.y >= pos2_min.y - 35);

	return overlap_x && overlap_y;

}


void PhysicsSystem::step(float elapsed_ms, WorldSystem* world)
{
	ComponentContainer<Motion>& motion_container = registry.motions;
	// Move entities based on the time passed, ensuring entities move at consistent speeds
	auto& motion_registry = registry.motions;
	for (uint i = 0; i < motion_registry.size(); i++)
	{


		Motion& motion = motion_registry.components[i];
		Entity entity = motion_registry.entities[i];
		float step_seconds = elapsed_ms / 1000.f;
		if (registry.robots.has(entity)) {
			dumb_ai(motion);
		}
		motion.velocity.x = linear_inter(motion.target_velocity.x, motion.velocity.x, step_seconds * 100.0f);
		motion.velocity.y = linear_inter(motion.target_velocity.y, motion.velocity.y, step_seconds * 100.0f);
		vec2 pos = motion.position;
		motion.position += motion.velocity * step_seconds;



		if (!registry.tiles.has(entity)) {

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
							motion.position = pos;
							motion.velocity = vec2(0);

							world->play_collision_sound();
						}
					}
				}
			}
			bound_check(motion);
		}
	}



	// Check for collisions between all moving entities
	for (uint i = 0; i < motion_container.components.size(); i++)
	{
		Motion& motion_i = motion_container.components[i];
		Entity entity_i = motion_container.entities[i];
		if (registry.tiles.has(entity_i)) {
			continue;
		}

		// note starting j at i+1 to compare all (i,j) pairs only once (and to not compare with itself)
		for (uint j = i + 1; j < motion_container.components.size(); j++)
		{
			Motion& motion_j = motion_container.components[j];
			Entity entity_j = motion_container.entities[j];
			if (registry.tiles.has(entity_j)) {
				continue;
			}
			if (collides(motion_i, motion_j))
			{
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
	mo.position.x = max(min((float)window_width_px - 16, mo.position.x), 0.f + 16);
	mo.position.y = max(min((float)window_height_px - 32, mo.position.y), 0.f);
}