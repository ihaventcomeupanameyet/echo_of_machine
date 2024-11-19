#include "json.hpp"
#include <iostream>
#include <fstream>
using json = nlohmann::json;



void generate_json(const ECSRegistry& rej)
{
	json j;

    j["id_count"] = Entity::id_count;
	j["attackBox"] = rej.attackbox;
    j["DeathTimer"] = rej.deathTimers;
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

void load_json(ECSRegistry& rej) {
    std::string s = PROJECT_SOURCE_DIR + std::string("/data/data.json");
    std::ifstream inFile(s);

    if (inFile.is_open()) {
        nlohmann::json j;
        inFile >> j;
        inFile.close();
        Entity::id_count = j.at("id_count").get<unsigned int>();
        from_json(j.at("attackBox"), rej.attackbox);
        from_json(j.at("DeathTimer"), rej.deathTimers);
        //from_json(j.at("collision"), rej.collisions);
        from_json(j.at("player"), rej.players);
        from_json(j.at("PlayerAnimation"), rej.animations);
        from_json(j.at("RobotAnimation"), rej.robotAnimations);
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

        std::cout << "JSON loaded successfully from " << s << std::endl;
    }
    else {
        std::cerr << "Failed to open the file for reading." << std::endl;
    }
}