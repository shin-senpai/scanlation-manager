// Associated Header Include
#include "bot/eventHandlers/commands/RegisterUser.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/User.hpp"

// Standard Includes
#include <cstdint>
#include <exception>
#include <string>

// Third Party Includes
#include <dpp/dispatcher.h>
#include <pqxx/pqxx>

void Commands::registerUser(Bot &bot, const dpp::slashcommand_t &event) {
  const int64_t discord_id = static_cast<int64_t>(event.command.usr.id);
  const std::string display_name = event.command.usr.username;

  try {
    DbSession session(bot.getPool());

    UserRepository user_repo;
    DiscordIdentityRepository identity_repo;

    const int user_id = user_repo.create(session.tx(), display_name);
    identity_repo.create(session.tx(), discord_id, user_id);

    session.commit();

    event.reply("You've been registered! Welcome, " + display_name + ".");

  } catch(const pqxx::unique_violation &) {
    event.reply("You're already registered.");
  } catch(const std::exception &) {
    event.reply("Registration failed. Please try again later.");
  }
}