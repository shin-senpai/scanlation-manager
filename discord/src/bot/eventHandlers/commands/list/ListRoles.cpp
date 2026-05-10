// Associated Header Include
#include "bot/eventHandlers/commands/list/ListRoles.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/Roles.hpp"

// Standard Includes
#include <iostream>
#include <string>

// Third Party Includes
#include <dpp/dispatcher.h>
#include <pqxx/pqxx>

void Commands::listRoles(Bot &bot, const dpp::slashcommand_t &event) {
  event.thinking(true);

  try {
    DbSession session(bot.getPool());
    RolesRepository roles_repo;

    const auto roles = roles_repo.listAll(session.rtx());

    if(roles.empty()) {
      event.edit_original_response(dpp::message("No roles have been created yet."));
      return;
    }

    std::string response = "**Roles:**\n";
    for(const auto &role : roles) {
      response += "- " + role.name + " (ID: `" + std::to_string(role.id) + "`)\n";
    }

    event.edit_original_response(dpp::message(response));
  } catch(const std::exception &e) {
    std::cerr << "listRoles failed: " << e.what() << std::endl;
    event.edit_original_response(dpp::message("Failed to retrieve roles. Contact the administrator to resolve this issue."));
  }
}
