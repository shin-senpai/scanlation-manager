#pragma once

// Standard Includes
#include <string>

// Third Party Includes
#include <dpp/dispatcher.h>

class Bot;

namespace Commands {
void workProgress(const dpp::slashcommand_t &event);
void workProgressAutocomplete(Bot &bot, const std::string &option_name, const std::string &input, const dpp::autocomplete_t &event);
}