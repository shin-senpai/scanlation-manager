// Associated Header Include
#include "bot/utils/GetAutoCompleteContext.hpp"
#include "appcommand.h"

// User Defined Includes

// Standard Includes
#include <string>

// Third Party Includes
#include <dpp/dispatcher.h>

std::string BotUtils::getAutoCompleteContext(const dpp::autocomplete_t &event, const std::string &option_name) {
  if(event.options.empty()) {
    return {};
  }
  const auto opts = (event.options[0].type == dpp::co_sub_command) ? event.options[0].options : event.options;

  for(const auto &opt : opts) {
    if(opt.name == option_name && !opt.focused) {
      try {
        return std::get<std::string>(opt.value);
      } catch(...) {
      }
    }
  }
  return {};
}