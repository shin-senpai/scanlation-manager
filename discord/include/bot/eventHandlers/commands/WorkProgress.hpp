#pragma once

// Standard Includes
#include <string_view>

// Third Party Includes
#include <dpp/dispatcher.h>

class Bot;

namespace Commands {
void workProgress(const dpp::slashcommand_t &event);
void workProgressAutocomplete(Bot &bot, const std::string_view &option_name, const std::string_view &input, const dpp::autocomplete_t &event);
}