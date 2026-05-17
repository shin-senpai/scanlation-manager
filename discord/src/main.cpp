// Associated Header Include

// User Defined Includes
#include "bot/Bot.hpp"
#include "utils/ConfigManager.hpp"
#include "utils/HttpUtils.hpp"

// Standard Includes

// Third Party Includes

/* When you invite the bot, be sure to invite it with the
 * scopes 'bot' and 'applications.commands', e.g.
 * https://discord.com/oauth2/authorize?client_id=940762342495518720&scope=bot+applications.commands&permissions=139586816064
 */

int main(int argc, char const *argv[]) {

  try {
    CurlGlobalManager::curlManagerInit();
    ConfigManager config("config.json");
    Bot scan_manager(config);
    scan_manager.start();
  } catch(const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return 0;
}
