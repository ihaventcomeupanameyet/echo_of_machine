// internal
#include "tiny_ecs.hpp"

// All we need to store besides the containers is the id of every entity and callbacks to be able to remove entities across containers
unsigned int Entity::id_count = 1;


void to_json(nlohmann::json& j, const Entity& entity) {
    j = nlohmann::json{ {"id", entity.id} };  
}


void from_json(const nlohmann::json& j, Entity& entity) {
    j.at("id").get_to(entity.id); 
}

