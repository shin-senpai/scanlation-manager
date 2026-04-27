// Associated Header Include
#include "bot/eventHandlers/commands/ListRoleTasks.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/RoleTasks.hpp"
#include "db/repositories/Roles.hpp"
#include "db/repositories/Tasks.hpp"

// Standard Includes
#include <iostream>
#include <string>
#include <vector>

// Third Party Includes
#include <dpp/dispatcher.h>
#include <pqxx/pqxx>

void Commands::listRoleTasks(Bot &bot, const dpp::slashcommand_t &event) {
  event.thinking(true);

  try {
    DbSession session(bot.getPool());
    RoleTasksRepository role_task_repo;
    RolesRepository role_repo;
    TasksRepository task_repo;

    const auto roles = role_repo.listAll(session.rtx());
    const auto tasks = task_repo.listAll(session.rtx());
    const auto role_task_maps = role_task_repo.listAll(session.rtx());

    std::unordered_map<int, std::string> role_name_by_id;
    std::unordered_map<int, std::string> task_name_by_id;
    for(const auto &r : roles) {
      role_name_by_id[r.id] = r.name;
    }
    for(const auto &t : tasks) {
      task_name_by_id[t.id] = t.name;
    }

    std::unordered_map<std::string, std::vector<std::string>> role_to_tasks;
    for(const auto &rtm : role_task_maps) {
      int role_id = rtm.role_id;
      int task_id = rtm.task_id;

      auto task_it = task_name_by_id.find(task_id);
      if(task_it == task_name_by_id.end()) {
        continue;
      }
      auto role_it = role_name_by_id.find(role_id);
      if(role_it == role_name_by_id.end()) {
        continue;
      }

      role_to_tasks[role_it->second].push_back(task_it->second);
    }

    std::string response = "**Role-Task Mappings:**\n";
    for(const auto &[role_name, task_names] : role_to_tasks) {
      response += "- **" + role_name + "** -> ";
      for(const auto &task_name : task_names) {
        response += "**" + task_name + "** ";
      }
      response += "\n";
    }

    event.edit_original_response(dpp::message(response));
  } catch(const std::exception &e) {
    std::cerr << "listRoleTasks failed: " << e.what() << std::endl;
    event.edit_original_response(dpp::message("Failed to retrieve RoleTasks. Contact the administrator to resolve this issue."));
  }
}