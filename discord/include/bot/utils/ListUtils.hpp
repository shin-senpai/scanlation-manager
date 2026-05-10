#pragma once

// Standard Includes
#include <string>
#include <vector>

namespace BotUtils {
std::vector<std::string> makePages(
    const std::string &title,
    const std::vector<std::string> &lines,
    size_t per_page = 10);
}
