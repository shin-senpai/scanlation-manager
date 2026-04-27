#pragma once

// Third Party Includes
#include <dpp/dispatcher.h>

class Bot;

namespace Commands {
  void listRoleTasks(Bot &bot, const dpp::slashcommand_t &event);
}