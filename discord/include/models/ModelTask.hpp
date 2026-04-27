#pragma once

// Standard Includes
#include <optional>
#include <string>

struct Task {
  int id;
  std::string name;
  std::optional<std::string> retired_at;
};
