//config.h - Contains the definition for the ConfigManager Class
#pragma once

#include <dpp/nlohmann/json.hpp>
#include <fstream>
#include <string>
#include <mutex>

using json = nlohmann::json;

class ConfigManager {
private:
    std::string path;
    json data;
    std::mutex mtx;

public:
    ConfigManager(const std::string& file_path) : path(file_path) {
        load();
    }

    void load() {
        std::lock_guard<std::mutex> lock(mtx);
        std::ifstream f(path);
        if (f.is_open()) {
            f >> data;
        }
    }

    void save() {
        std::lock_guard<std::mutex> lock(mtx);
        std::ofstream f(path);
        f << data.dump(4);
    }

    // Template to get any value easily
    template<typename T>
    T get(const std::string& key, T default_val = T()) {
        return data.value(key, default_val);
    }

    void set(const std::string& key, const json& value) {
        data[key] = value;
        save();
    }
};