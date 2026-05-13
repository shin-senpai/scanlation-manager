#pragma once

// Standard Includes
#include <optional>
#include <string>

struct UserHistoryEntry {
  std::string series_name;
  std::optional<int> volume;
  double chapter_number;
  std::optional<std::string> chapter_name;
  std::string task_name;
  std::string completed_at; // YYYY-MM-DD
};
