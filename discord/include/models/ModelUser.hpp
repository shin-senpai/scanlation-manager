#pragma once
 
// User Defined Includes
#include "types/Permission.hpp"

// Standard Includes
#include <optional>
#include <string>

struct User {
  int id;
  std::optional<std::string> name;  // nullable — Discord users may not have a webapp username
  std::string display_name;
  std::string joined_at;
  std::optional<std::string> left_at;
  Permission permission_level;
};