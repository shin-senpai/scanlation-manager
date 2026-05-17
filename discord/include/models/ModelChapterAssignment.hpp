#pragma once

// Standard Includes
#include <optional>
#include <string>

struct ChapterAssignment {
  int user_id;
  int chapter_id;
  int task_id;
  std::string assigned_at;
  std::optional<std::string> completed_at;
};
