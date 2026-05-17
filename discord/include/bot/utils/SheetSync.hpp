#pragma once

// Standard Includes
#include <string>

class Bot;
namespace SheetSync {
void syncSeries(Bot &bot, const std::string &series_name);
void syncTodo(Bot &bot);
void deleteSeries(Bot &bot, const std::string &series_name);
} // namespace SheetSync
