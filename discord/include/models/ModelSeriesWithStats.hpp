#pragma once

// User Defined Includes
#include "types/SeriesStatus.hpp"

// Standard Includes
#include <optional>
#include <string>

struct SeriesWithStats {
  int id;
  std::string name;
  SeriesStatus status;
  std::string added_at;
  std::optional<std::string> closed_at;
  int chapter_count;
  std::optional<std::string> latest_chapter_at; // formatted YYYY-MM-DD, nullopt if no chapters matched
};
