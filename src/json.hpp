#pragma once

#ifndef JSON_UTILS_HPP
#define JSON_UTILS_HPP

#include "../ext/json.hpp"
#include "components.hpp"
#include "tiny_ecs_registry.hpp"
#include "world_system.hpp"

void generate_json(const ECSRegistry& rej, const WorldSystem& wor);

void load_json(ECSRegistry& rej, WorldSystem& wor);


#endif
