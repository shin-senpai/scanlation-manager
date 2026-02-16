#pragma once

#include <dpp/dpp.h> 
#include <string>
#include <map>
#include <functional>

class ConfigManager; 
class Bot {
private:
    dpp::cluster core;
    ConfigManager& config;
    dpp::snowflake work_progress_channel{};
    
    struct CommandInfo {
        std::string description;
        std::function<void(const dpp::slashcommand_t&)> handler;
        std::vector<dpp::command_option> options = {};
    };

    std::map<std::string, CommandInfo> commands;
    const dpp::snowflake GUILD_ID = 1254062114159726693;

public:
    Bot(const std::string& token, ConfigManager& cfg);
    
    void start();

private:
    void registerCommands(); 

    void commandPing(const dpp::slashcommand_t& event);
    void commandSetProgressChannel(const dpp::slashcommand_t& event);
};