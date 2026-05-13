#pragma once

// User Defined Includes
#include "types/ChapterStatus.hpp"

// Standard Includes
#include <optional>
#include <string>

struct ChapterWithStats {
  int id;
  int series_id;
  std::string series_name;
  std::optional<int> volume;
  double number;
  std::optional<std::string> name;
  ChapterStatus status;
  std::string added_at;
  std::optional<std::string> closed_at; // formatted YYYY-MM-DD
  int total_tasks;                       // distinct task_ids assigned
  int completed_tasks;                   // distinct task_ids with >= 1 completed assignment
};
