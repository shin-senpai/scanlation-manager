#pragma once

// Standard Includes
#include <string>
#include <vector>

namespace BotUtils {
// Parses a comma-separated string of chapter numbers.
// Trims whitespace, validates non-negative, deduplicates (preserves first-seen order).
// Sets error and returns {} on any problem.
std::vector<double> parseChapterNumbers(const std::string &input, std::string &error);
} // namespace BotUtils
