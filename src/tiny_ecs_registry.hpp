#pragma once
#include <vector>

#include "tiny_ecs.hpp"
#include "components.hpp"

class ECSRegistry
{
	// Callbacks to remove a particular or all entities in the system
	std::vector<ContainerInterface*> registry_list;

public:
	// Manually created list of all components this game has
	// TODO: A1 add a LightUp component
	ComponentContainer<DeathTimer> deathTimers;
	ComponentContainer<Motion> motions;
	ComponentContainer<Collision> collisions;
	ComponentContainer<Player> players;
	ComponentContainer<PlayerAnimation> animations;
	ComponentContainer<RobotAnimation> robotAnimations;
	ComponentContainer<Mesh*> meshPtrs;
	ComponentContainer<RenderRequest> renderRequests;
	ComponentContainer<ScreenState> screenStates;
	ComponentContainer<Robot> robots;
	ComponentContainer<Tile> tiles;
	ComponentContainer<TileSetComponent> tilesets;
	ComponentContainer<Key> keys;
	ComponentContainer<ArmorPlate> armorplates;
	ComponentContainer<Potion> potions;

	ComponentContainer<DebugComponent> debugComponents;
	ComponentContainer<vec3> colors;

	ComponentContainer<T_map> maps;

	ComponentContainer<attackBox> attackbox;

	ComponentContainer<Spaceship> spaceships;


	// constructor that adds all containers for looping over them
	// IMPORTANT: Don't forget to add any newly added containers!
	ECSRegistry()
	{
		registry_list.push_back(&deathTimers);
		registry_list.push_back(&motions);
		registry_list.push_back(&collisions);
		registry_list.push_back(&players);
		registry_list.push_back(&animations);
		registry_list.push_back(&robotAnimations);
		registry_list.push_back(&meshPtrs);
		registry_list.push_back(&renderRequests);
		registry_list.push_back(&screenStates);
		registry_list.push_back(&robots);
		registry_list.push_back(&tiles);
		registry_list.push_back(&debugComponents);
		registry_list.push_back(&colors);
		registry_list.push_back(&maps);
		registry_list.push_back(&keys);
		registry_list.push_back(&potions);

		registry_list.push_back(&attackbox);

		registry_list.push_back(&armorplates);

		registry_list.push_back(&spaceships);

	}

	void clear_all_components() {
		for (ContainerInterface* reg : registry_list)
			reg->clear();
	}

	void list_all_components() {
		printf("Debug info on all registry entries:\n");
		for (ContainerInterface* reg : registry_list)
			if (reg->size() > 0)
				printf("%4d components of type %s\n", (int)reg->size(), typeid(*reg).name());
	}

	void list_all_components_of(Entity e) {
		printf("Debug info on components of entity %u:\n", (unsigned int)e);
		for (ContainerInterface* reg : registry_list)
			if (reg->has(e))
				printf("type %s\n", typeid(*reg).name());
	}

	void remove_all_components_of(Entity e) {
		for (ContainerInterface* reg : registry_list)
			reg->remove(e);
	}
};

extern ECSRegistry registry;