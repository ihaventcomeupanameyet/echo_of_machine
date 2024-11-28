#pragma once

#include <algorithm>
#include <vector>
#include <unordered_map>
#include <set>
#include <functional>
#include <typeindex>
#include <assert.h>

#include "../ext/json.hpp"

using json = nlohmann::json;
// Unique identifyer for all entities
class Entity
{
	//unsigned int id;
	//static unsigned int id_count; // starts from 1, entit 0 is the default initialization
public:
	unsigned int id;
	static unsigned int id_count;
	Entity()
	{
		id = id_count++;
		// Note, indices of already deleted entities arent re-used in this simple implementation.
	}
	operator unsigned int() { return id; } // this enables automatic casting to int
};

// Common interface to refer to all containers in the ECS registry
struct ContainerInterface
{
	virtual void clear() = 0;
	virtual size_t size() = 0;
	virtual void remove(Entity e) = 0;
	virtual bool has(Entity entity) = 0;
};


template <typename Component> // A component can be any class
class ComponentContainer : public ContainerInterface
{
private:
	// The hash map from Entity -> array index.
	// std::unordered_map<unsigned int, unsigned int> map_entity_componentID; // the entity is cast to uint to be hashable.
	bool registered = false;
public:
	// Container of all components of type 'Component'
	std::unordered_map<unsigned int, unsigned int> map_entity_componentID;
	std::vector<Component> components;

	// The corresponding entities
	std::vector<Entity> entities;

	// Constructor that registers the type
	ComponentContainer()
	{
	}

	// Inserting a component c associated to entity e
	inline Component& insert(Entity e, Component c, bool check_for_duplicates = true)
	{
		// Usually, every entity should only have one instance of each component type
		//assert(!(check_for_duplicates && has(e)) && "Entity already contained in ECS registry");

		const char* component_type = typeid(Component).name();
		if (check_for_duplicates && has(e)) {
			std::cout << "Assertion failed: Entity already contained in ECS registry for component type: "
				<< component_type << "\n";
			assert(false); // Trigger the assertion failure explicitly
		}

		map_entity_componentID[e] = (unsigned int)components.size();
		components.push_back(std::move(c)); // the move enforces move instead of copy constructor
		entities.push_back(e);
		return components.back();
	};

	// The emplace function takes the the provided arguments Args, creates a new object of type Component, and inserts it into the ECS system
	template<typename... Args>
	Component& emplace(Entity e, Args &&... args) {
		return insert(e, Component(std::forward<Args>(args)...));
	};
	template<typename... Args>
	Component& emplace_with_duplicates(Entity e, Args &&... args) {
		return insert(e, Component(std::forward<Args>(args)...), false);
	};

	// A wrapper to return the component of an entity
	Component& get(Entity e) {
		const char* component_type = typeid(Component).name();
		//assert(!(check_for_duplicates && has(e)) &&
			//("Entity already contained in ECS registry for component type: " + std::string(component_type)).c_str());

		if (!has(e)) {
			std::cout << "Entity not contained in ECS registry "
				<< component_type << "\n";
			assert(false); // Trigger the assertion failure explicitly
		}
		assert(has(e) && "Entity not contained in ECS registry");
		return components[map_entity_componentID[e]];
	}

	// Check if entity has a component of type 'Component'
	bool has(Entity entity) {
		return map_entity_componentID.count(entity) > 0;
	}

	// Remove an component and pack the container to re-use the empty space
	void remove(Entity e)
	{
		if (has(e))
		{
			// Get the current position
			int cID = map_entity_componentID[e];

			// Move the last element to position cID using the move operator
			// Note, components[cID] = components.back() would trigger the copy instead of move operator
			components[cID] = std::move(components.back());
			entities[cID] = entities.back(); // the entity is only a single index, copy it.
			map_entity_componentID[entities.back()] = cID;

			// Erase the old component and free its memory
			map_entity_componentID.erase(e);
			components.pop_back();
			entities.pop_back();
			// Note, one could mark the id for re-use
		}
	};

	// Remove all components of type 'Component'
	void clear()
	{
		map_entity_componentID.clear();
		components.clear();
		entities.clear();
	}

	// Report the number of components of type 'Component'
	size_t size()
	{
		return components.size();
	}

	// Sort the components and associated entity assignment structures by the comparisonFunction, see std::sort
	template <class Compare>
	void sort(Compare comparisonFunction)
	{
		// First sort the entity list as desired
		std::sort(entities.begin(), entities.end(), comparisonFunction);
		// Now re-arrange the components (Note, creates a new vector, which may be slow! Not sure if in-place could be faster: https://stackoverflow.com/questions/63703637/how-to-efficiently-permute-an-array-in-place-using-stdswap)
		std::vector<Component> components_new; components_new.reserve(components.size());
		std::transform(entities.begin(), entities.end(), std::back_inserter(components_new), [&](Entity e) { return std::move(get(e)); }); // note, the get still uses the old hash map (on purpose!)
		components = std::move(components_new); // note, we use move operations to not create unneccesary copies of objects, but memory is still allocated for the new vector
		// Fill the new hashmap
		for (unsigned int i = 0; i < entities.size(); i++)
			map_entity_componentID[entities[i]] = i;
	}
};



void to_json(nlohmann::json& j, const Entity& entity);
void from_json(const nlohmann::json& j, Entity& entity);
template <typename Component>
void to_json(nlohmann::json& j, const ComponentContainer<Component>& container) {
	j = nlohmann::json{
		{"components", container.components},
		{"entities", container.entities},
		{"map_entity_componentID", container.map_entity_componentID}
	};
}

template <typename Component>
void from_json(const nlohmann::json& j, ComponentContainer<Component>& container) {
	j.at("components").get_to(container.components);
	j.at("entities").get_to(container.entities);
	j.at("map_entity_componentID").get_to(container.map_entity_componentID);
}