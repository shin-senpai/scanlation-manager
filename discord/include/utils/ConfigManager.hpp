// ConfigManager.h - Contains the definition for the ConfigManager Class
#pragma once

// Standard Includes
#include <mutex>
#include <string>
#include <vector>

// Third Party Includes
#include <nlohmann/json.hpp>

class ConfigManager {
private:
  std::string m_path;
  mutable std::mutex m_mtx;
  nlohmann::json m_data;

  void save();

public:
  explicit ConfigManager(const std::string &file_path);

  void load();

  void set(const std::string &key, const nlohmann::json &value);

  void setMultiple(const std::vector<std::string> &keys,
                   const std::vector<nlohmann::json> &values);

  template <typename T>
  inline T getOptional(const std::string &key, T default_val = T()) const {
    std::lock_guard<std::mutex> lock(m_mtx);
    return m_data.value(key, default_val);
  }

  template <typename T>
  inline T getRequired(const std::string &key) const {
    std::lock_guard<std::mutex> lock(m_mtx);
    return m_data.at(key).get<T>();
  }
};