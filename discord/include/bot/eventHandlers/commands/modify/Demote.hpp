#pragma once

// Third Party Includes
#include <dpp/dispatcher.h>

class Bot;

namespace Commands {
void demote(Bot &bot, const dpp::slashcommand_t &event);
} // namespace Commands
