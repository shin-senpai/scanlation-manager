#pragma once

// Standard Includes
#include <string>

// Third Party Includes
#include <dpp/dispatcher.h>

class Bot;
namespace Commands {
void removeTaskDep(Bot &bot, const dpp::slashcommand_t &event);
void removeTaskDepAutocomplete(Bot &bot, const std::string &key, const std::string &input, const dpp::autocomplete_t &event);
}
