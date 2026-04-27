// Associated Header Include
#include "bot/eventHandlers/commands/ListTasks.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/Tasks.hpp"

// Standard Includes
#include <iostream>
#include <string>

// Third Party Includes
#include <dpp/dispatcher.h>
#include <pqxx/pqxx>

void Commands::listTasks(Bot &bot, const dpp::slashcommand_t &event) {
  event.thinking(true);

  try {
    DbSession session(bot.getPool());
    TasksRepository tasks_repo;

    const auto tasks = tasks_repo.listAll(session.rtx());

    if(tasks.empty()) {
      event.edit_original_response(dpp::message("No tasks have been created yet."));
      return;
    }

    std::string response = "**Tasks:**\n";
    for(const auto &task : tasks) {
      response += "- " + task.name + " (ID: `" + std::to_string(task.id) + "`)\n";
    }

    event.edit_original_response(dpp::message(response));
  } catch(const std::exception &e) {
    std::cerr << "listTasks failed: " << e.what() << std::endl;
    event.edit_original_response(dpp::message("Failed to retrieve tasks. Contact the administrator to resolve this issue."));
  }
}
