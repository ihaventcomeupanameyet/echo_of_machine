#include "json.hpp"
#include <iostream>
#include <fstream>
using json = nlohmann::json;



void generate_json(const ECSRegistry& rej)
{
	json j;
	json j_atk = rej.attackbox;

	j["attackBox"] = j_atk;
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
	(void)rej;

}