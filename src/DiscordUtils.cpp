//Associated Header Include
#include "DiscordUtils.h"

//User Defined Includes

//Standard Includes
#include <string>
#include <vector>

//Third Party Includes
#include <dpp/dpp.h>
#include <dpp/appcommand.h>
#include <dpp/dispatcher.h>
#include <dpp/nlohmann/json.hpp>

using command_handler = std::function<void(const dpp::slashcommand_t&)>;

struct CommandInfo {
    std::string description;
    command_handler handler;
};

void handlePingCommand(const dpp::slashcommand_t& event) {
    event.reply("Pong! 🏓");
}

std::map<std::string, CommandInfo> commands = {
{"ping", {"Ping Pong", handlePingCommand}}
};

void initBot(const std::string& DISCORD_BOT_TOKEN){
    /* Set up the bot */
    dpp::cluster bot(DISCORD_BOT_TOKEN);

    /* Output simple log messages to stdout */
    bot.on_log(dpp::utility::cout_logger());

    bot.on_ready([&bot](const dpp::ready_t& event) {
        /* Wrap command registration in run_once to make sure it doesn't run on every full reconnection */
        if (dpp::run_once<struct register_bot_commands>()) {

            std::vector<dpp::slashcommand> slash_commands;

            for (const auto& command : commands){
                slash_commands.push_back(dpp::slashcommand(command.first, command.second.description, bot.me.id));
            }

            bot.global_bulk_command_create(slash_commands);
        }
    });

    bot.on_slashcommand([](const dpp::slashcommand_t& event){
        auto it = commands.find(event.command.get_command_name());
        if (it != commands.end()){
            it->second.handler(event);
        }
    });

    bot.start(dpp::st_wait);
}