#pragma once

// Third Party Includes
#include <dpp/dispatcher.h>

class Bot;

namespace Commands {
  void removeRoleTask(Bot &bot, const dpp::slashcommand_t &event);
}
