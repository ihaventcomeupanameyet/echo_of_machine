// internal
#include "physics_system.hpp"
#include "world_init.hpp"

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


void PhysicsSystem::step(float elapsed_ms)
{
	ComponentContainer<Motion>& motion_container = registry.motions;
	// Move entities based on the time passed, ensuring entities move at consistent speeds
	auto& motion_registry = registry.motions;
	for (uint i = 0; i < motion_registry.size(); i++)
	{
		Motion& motion = motion_registry.components[i];
		Entity entity = motion_registry.entities[i];
		float step_seconds = elapsed_ms / 1000.f;
		vec2 pos = motion.position;
		motion.position += motion.velocity * step_seconds;
		vec2& position = motion.position;


		Motion& motion_i = motion_container.components[i];
		Entity entity_i = motion_container.entities[i];
		for (uint j = i + 1; j < motion_container.components.size(); j++)
		{
			//Motion& motion_i = motion_container.components[i];
			//Entity entity_i = motion_container.entities[i];
			Motion& motion_j = motion_container.components[j];
			if (collides(motion_i, motion_j))
			{
				Entity entity_j = motion_container.entities[j];

				// Set velocities to zero when collision happens
				motion_i.velocity = { 0.f, 0.f };
				motion_j.velocity = { 0.f, 0.f };

				// Create collision events
				registry.collisions.emplace_with_duplicates(entity_i, entity_j);
				registry.collisions.emplace_with_duplicates(entity_j, entity_i);
				position = pos;

			}
		}
		// Clamp the position to the screen boundaries
		position.x = fmax(16.f, fmin(position.x, window_width_px-16));
		position.y = fmax(0.f, fmin(position.y, window_height_px-32));
	}

	// Check for collisions between all moving entities

	for (uint i = 0; i < motion_container.components.size(); i++)
	{
		Motion& motion_i = motion_container.components[i];
		Entity entity_i = motion_container.entities[i];

		// Note: starting j at i+1 to compare all (i,j) pairs only once (and to avoid self-comparison)
		for (uint j = i + 1; j < motion_container.components.size(); j++)
		{
			Motion& motion_j = motion_container.components[j];
			if (collides(motion_i, motion_j))
			{
				Entity entity_j = motion_container.entities[j];

				// Set velocities to zero when collision happens
				motion_i.velocity = { 0.f, 0.f };
				motion_j.velocity = { 0.f, 0.f };

				// Create collision events
				registry.collisions.emplace_with_duplicates(entity_i, entity_j);
				registry.collisions.emplace_with_duplicates(entity_j, entity_i);


			}
		}
	}
}
