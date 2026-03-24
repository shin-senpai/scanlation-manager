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
  CurlGlobalManager::curlManagerInit();

  ConfigManager config("config.json");

  const std::string gsheet_auth_token = config.getOptional<std::string>("gsheet_auth_token");
  const std::string gsheet_priv_api_url = config.getOptional<std::string>("gsheet_priv_api_url");

  // // --- GOOGLE SHEETS API TEST START ---
  // std::cout << "[Google Sheets] Testing connection..." << std::endl;

  // // 1. Construct the Payload
  // nlohmann::json payload;
  // payload["auth"] = gsheet_auth_token;
  // payload["action"] = "info"; // Simple action to check connectivity
  // // payload["spreadsheet_id"] = "..."; // Optional: Only if not hardcoded in
  // Script

  // // 2. Prepare Request
  // std::string response_buffer;
  // std::vector<std::string> headers = { "Content-Type: application/json" };

  // // 3. Send POST Request
  // // Note: We use payload.dump() to convert the JSON object to a std::string
  // int http_code = httpPost(gsheet_priv_api_url, headers, payload.dump(),
  // response_buffer);

  // // 4. Output Result
  // if (http_code == 200) {
  //     std::cout << "[Google Sheets] Success! Response:\n" << response_buffer
  //     << std::endl;
  // } else {
  //     std::cerr << "[Google Sheets] Failed. HTTP Code: " << http_code <<
  //     "\nResponse: " << response_buffer << std::endl;
  // }
  // // --- GOOGLE SHEETS API TEST END ---

  try {
    Bot scan_manager(config);
    scan_manager.start();
  } catch(const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return 0;
}
