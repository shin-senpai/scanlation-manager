#pragma once

// Standard Includes
#include <string>

// Third Party Includes
#include <dpp/dispatcher.h>

namespace BotUtils {
std::string getAutoCompleteContext(const dpp::autocomplete_t &event, const std::string &option_name);
}