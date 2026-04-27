#pragma once

// Third Party Includes
#include <dpp/dispatcher.h>

class Bot;

namespace Commands {
  void listTasks(Bot &bot, const dpp::slashcommand_t &event);
}
