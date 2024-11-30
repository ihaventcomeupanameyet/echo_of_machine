#pragma once

#include <vector>

#include "tiny_ecs_registry.hpp"
#include "common.hpp"

class AISystem
{
public:
	void step(float elapsed_ms);

private:
    vec2 calculateSeparation(Entity entity);
    vec2 calculateAlignment(Entity entity);
    vec2 calculateCohesion(Entity entity);
    vec2 chasePlayer(Entity entity);
    vec2 calculateWander(Entity entity);
};