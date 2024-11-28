#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "components.hpp"
#include "tiny_ecs_registry.hpp"
#include "world_system.hpp"

// A simple physics system that moves rigid bodies and checks for collision
class PhysicsSystem
{
public:
	void step(float elapsed_ms, WorldSystem* world);

	PhysicsSystem()
	{
	}
private: 
	bool checkMeshCollision(const Motion& motion1, const Motion& motion2, const Mesh* mesh);
	
};


struct Triangle {
    vec2 v1, v2, v3; // vertices of the triangle
};