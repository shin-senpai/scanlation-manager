#pragma once
 
// Standard Includes
#include <optional>
#include <string>
 
struct User {
  int id;
  std::optional<std::string> name;  // nullable — Discord users may not have a webapp username
  std::string display_name;
  bool is_manager;
  bool is_supermanager;
};