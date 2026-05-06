#pragma once

// Standard Includes
#include <optional>
#include <string>

struct CrewDetail {
  std::string task_name;
  std::string user_display;
};

struct AssignmentDetail {
  std::string task_name;
  std::string user_display;
  std::optional<std::string> completed_at; // YYYY-MM-DD, nullopt if outstanding
};
