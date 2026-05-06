#pragma once

// Standard Includes
#include <string>

namespace BotUtils {
// Converts a date string (YYYY-MM-DD or full timestamp) to a Discord timestamp: <t:UNIX>
std::string toDiscordTimestamp(const std::string &date);
} // namespace BotUtils
