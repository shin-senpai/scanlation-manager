// Associated Header Include
#include "utils/ConfigManager.hpp"

// User Defined Includes

// Standard Includes
#include <fstream>
#include <stdexcept>

// Third Party Includes
#include <nlohmann/json.hpp>

ConfigManager::ConfigManager(const std::string &file_path) : m_path(file_path) {
  load();
}

// Save is an internal method, and can/should only be called while holding the mutex
void ConfigManager::save() {
  std::ofstream f(m_path);
  f << m_data.dump(4);
}

void ConfigManager::load() {
  std::lock_guard<std::mutex> lock(m_mtx);
  std::ifstream f(m_path);
  if(f.is_open() && f.peek() != std::ifstream::traits_type::eof()) {
    try {
      f >> m_data;
    } catch(const nlohmann::json::exception &e) {
      m_data = nlohmann::json::object();
    }
  } else {
    m_data = nlohmann::json::object();
  }
}

void ConfigManager::set(const std::string &key, const nlohmann::json &value) {
  std::lock_guard<std::mutex> lock(m_mtx);
  m_data[key] = value;
  save();
}

void ConfigManager::setMultiple(const std::vector<std::string> &keys, const std::vector<nlohmann::json> &values) {
  if(keys.size() != values.size()) {
    throw ::std::invalid_argument("Keys and Values must have the same size");
  }

  std::lock_guard<std::mutex> lock(m_mtx);

  for(std::size_t i = 0; i < keys.size(); ++i) {
    m_data[keys[i]] = values[i];
  }
  save();
}