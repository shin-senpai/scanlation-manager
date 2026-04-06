#pragma once

// User Defined Includes
#include "types/SeriesStatus.hpp"

// Standard Includes
#include <optional>
#include <string>

struct Series {
  int id;
  std::string name;
  SeriesStatus status;
  std::string added_at;
  std::optional<std::string> closed_at;
};
