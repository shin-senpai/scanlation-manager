#pragma once

// User Defined Includes
#include "types/ChapterStatus.hpp"

// Standard Includes
#include <optional>
#include <string>

struct Chapter {
  int id;
  int series_id;
  std::string name;
  ChapterStatus status;
  std::string added_at;
  std::optional<std::string> closed_at;
};
