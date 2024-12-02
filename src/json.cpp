#include "json.hpp"
#include <iostream>
#include <fstream>
using json = nlohmann::json;


std::vector<std::string> required_keys = {
    "world",
    "map_height",
    "map_width",
    "id_count",
    "attackBox",
    "DeathTimer",
    "IceAni",
    "bossProjectile",
    "bossRobotAnimations",
    "bossRobots",
    "doorAnimations",
    "doors",
    "player",
    "PlayerAnimation",
    "RobotAnimation",
    "RenderRequest",
    "ScreenState",
    "Robot",
    "Tile",
    "TileSetComponent",
    "keys",
    "armorplates",
    "potions",
    "debugComponents",
    "colors",
    "maps",
    "spaceships",
    "projectile",
    "motion",
    "notifications",
    "spiderRobots",
    "spiderRobotAnimations"
};

bool all_key_present(json j) {
    for (std::string key : required_keys) {
        if (!j.contains(key)) {
            std::cout << key<<std::endl;
            return false;
        }
    }
    return true;
}

void generate_json(const ECSRegistry& rej, const WorldSystem& wor)
{
	json j;


    j["notifications"] = rej.notifications;
    j["world"] = wor;
    j["map_height"] = map_height;
    j["map_width"] = map_width;
    j["id_count"] = Entity::id_count;
	j["attackBox"] = rej.attackbox;
    j["DeathTimer"] = rej.deathTimers;
    j["IceAni"] = rej.iceRobotAnimations;

    j["bossProjectile"] = rej.bossProjectile;
    j["bossRobotAnimations"] = rej.bossRobotAnimations;
    j["bossRobots"] = rej.bossRobots;
    j["doorAnimations"] = rej.doorAnimations;
    j["doors"] = rej.doors;

    //j["collision"] = rej.collisions;
    j["player"] = rej.players;
    j["PlayerAnimation"] = rej.animations;
    j["RobotAnimation"] = rej.robotAnimations;
    j["RenderRequest"] = rej.renderRequests;
    j["ScreenState"] = rej.screenStates;
    j["Robot"] = rej.robots;
    j["Tile"] = rej.tiles;
    j["TileSetComponent"] = rej.tilesets;

    j["keys"] = rej.keys;
    j["armorplates"] = rej.armorplates;
    j["potions"] = rej.potions;

    j["debugComponents"] = rej.debugComponents;
    j["colors"] = rej.colors;
    j["maps"] = rej.maps;

    j["spaceships"] = rej.spaceships;
    j["projectile"] = rej.projectile;

    j["motion"] = rej.motions;
    j["spiderRobots"] = rej.spiderRobots;
    j["spiderRobotAnimations"] = rej.spiderRobotAnimations;
    std::string s = PROJECT_SOURCE_DIR + std::string("/data/data.json");
    std::ofstream outFile(s);

    if (outFile.is_open()) {
        outFile << j.dump(4); 
        outFile.close();
        std::cout << "JSON saved to " << "../data/data.json" << std::endl;
    }
    else {
        std::cerr << "Failed to open the file for writing."<<std::endl;
    }
}

void load_json(ECSRegistry& rej, WorldSystem& wor) {
    std::string s = PROJECT_SOURCE_DIR + std::string("/data/data.json");
    std::ifstream inFile(s);

    if (inFile.is_open()) {
        nlohmann::json j;
        inFile >> j;
        inFile.close();
        if (!all_key_present(j)) {
            return;
        }
        from_json(j.at("world"), wor);
        map_height = j["map_height"];
        map_width = j["map_width"];
        Entity::id_count = j.at("id_count").get<unsigned int>();
        from_json(j.at("attackBox"), rej.attackbox);
        from_json(j.at("DeathTimer"), rej.deathTimers);
        //from_json(j.at("collision"), rej.collisions);
        from_json(j.at("IceAni"), rej.iceRobotAnimations);
        from_json(j.at("player"), rej.players);
        from_json(j.at("PlayerAnimation"), rej.animations);
        from_json(j.at("RobotAnimation"), rej.robotAnimations);
        from_json(j.at("notifications"), rej.notifications);
        from_json(j.at("spiderRobotAnimations"), rej.spiderRobotAnimations);

        from_json(j.at("bossProjectile"), rej.bossProjectile);
        from_json(j.at("bossRobotAnimations"), rej.bossRobotAnimations);
        from_json(j.at("bossRobots"), rej.bossRobots);
        from_json(j.at("doors"), rej.doors);
        from_json(j.at("doorAnimations"), rej.doorAnimations);

        from_json(j.at("RenderRequest"), rej.renderRequests);
        from_json(j.at("ScreenState"), rej.screenStates);
        from_json(j.at("Robot"), rej.robots);
        from_json(j.at("Tile"), rej.tiles);
        from_json(j.at("TileSetComponent"), rej.tilesets);
        from_json(j.at("keys"), rej.keys);
        from_json(j.at("armorplates"), rej.armorplates);
        from_json(j.at("potions"), rej.potions);
        from_json(j.at("debugComponents"), rej.debugComponents);
        from_json(j.at("colors"), rej.colors);
        from_json(j.at("maps"), rej.maps);
        from_json(j.at("spaceships"), rej.spaceships);
        from_json(j.at("projectile"), rej.projectile);
        from_json(j.at("motion"), rej.motions);
        from_json(j.at("spiderRobots"), rej.spiderRobots);
        std::cout << "JSON loaded successfully from " << s << std::endl;
    }
    else {
        std::cerr << "Failed to open the file for reading." << std::endl;
    }
}