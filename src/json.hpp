#pragma once

#ifndef JSON_UTILS_HPP
#define JSON_UTILS_HPP

#include "../ext/json.hpp"
#include "components.hpp"
#include "tiny_ecs_registry.hpp"


void generate_json(const ECSRegistry& rej);

void load_json(ECSRegistry& rej);


#endif
