#pragma once
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class DBConfig
{
public:
    bool LoadConfig(const std::string& filename);

    std::string host;
    std::string user;
    std::string password;
    std::string dbname;
    unsigned int port;
};

bool DBConfig::LoadConfig(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open config file: " << filename << std::endl;
        return false;
    }

    json config;
    file >> config;

    host = config["database"]["host"];
    user = config["database"]["user"];
    password = config["database"]["password"];
    dbname = config["database"]["dbname"];
    port = config["database"]["port"];

    return true;
}