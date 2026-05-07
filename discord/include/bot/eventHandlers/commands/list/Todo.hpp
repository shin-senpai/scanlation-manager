#pragma once

// Standard Includes
#include "dispatcher.h"

class Bot;
namespace Commands {
void todo(Bot &bot, const dpp::slashcommand_t event);
}