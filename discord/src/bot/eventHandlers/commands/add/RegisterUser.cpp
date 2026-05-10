// Associated Header Include
#include "bot/eventHandlers/commands/add/RegisterUser.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/User.hpp"
#include "types/Permission.hpp"

// Standard Includes
#include <cstdint>
#include <exception>
#include <string>
#include <variant>

// Third Party Includes
#include <dpp/dispatcher.h>
#include <pqxx/pqxx>

void Commands::registerUser(Bot &bot, const dpp::slashcommand_t &event) {
  event.thinking(true);
  const int64_t discord_id = static_cast<int64_t>(event.command.usr.id);

  try {
    DbSession session(bot.getPool());
    UserRepository user_repo;
    DiscordIdentityRepository identity_repo;

    const auto &param = event.get_parameter("user");
    dpp::snowflake target_snowflake{};
    if(const auto p = std::get_if<dpp::snowflake>(&param)) {
      target_snowflake = *p;
    }

    if(!target_snowflake.empty()) {
      const auto maybe_caller_id = identity_repo.findUserIdByDiscordId(session.wtx(), discord_id);
      if(!maybe_caller_id) {
        event.edit_original_response(dpp::message("You are not registered. Please run /register first."));
        return;
      }
      if(user_repo.getPermissionLevel(session.wtx(), *maybe_caller_id) < Permission::manager) {
        event.edit_original_response(dpp::message("You lack the permission to register other users."));
        return;
      }

      const dpp::user &target_user = event.command.get_resolved_user(target_snowflake);
      const int new_user_id = user_repo.create(session.wtx(), target_user.username);
      identity_repo.create(session.wtx(), static_cast<int64_t>(target_snowflake), new_user_id);
      session.commit();

      event.edit_original_response(dpp::message(target_user.username + " has been registered."));
    } else {
      const std::string display_name = event.command.usr.username;
      const bool first_user = user_repo.listUsers(session.wtx()).empty();
      const int user_id = user_repo.create(session.wtx(), display_name, first_user ? Permission::supermanager : Permission::standard);
      identity_repo.create(session.wtx(), discord_id, user_id);
      session.commit();

      event.edit_original_response(dpp::message("You've been registered! Welcome, " + display_name + "."));
    }
  } catch(const pqxx::unique_violation &) {
    event.edit_original_response(dpp::message("That user is already registered."));
  } catch(const std::exception &e) {
    event.edit_original_response(dpp::message("Registration failed. Please try again later."));
    std::cerr << "User Registration failed due to exception: " << e.what() << std::endl;
  }
}