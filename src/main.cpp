#include "Config.h"
#include "HttpUtils.h"
#include "DiscordUtils.h"

/* When you invite the bot, be sure to invite it with the
 * scopes 'bot' and 'applications.commands', e.g.
 * https://discord.com/oauth2/authorize?client_id=940762342495518720&scope=bot+applications.commands&permissions=139586816064
 */

using json = nlohmann::json;

int main(int argc, char const *argv[]) {
    CurlGlobalManager curl_lifecycle;

    ConfigManager config("config.json");

    const std::string DISCORD_BOT_TOKEN = config.get<std::string>("discord_bot_token");
    const std::string GSHEET_AUTH_TOKEN = config.get<std::string>("gsheet_auth_token");
    const std::string GSHEET_PRIV_API_URL = config.get<std::string>("gsheet_priv_api_url");
    
    // --- GOOGLE SHEETS API TEST START ---
    std::cout << "[Google Sheets] Testing connection..." << std::endl;

    // 1. Construct the Payload
    json payload;
    payload["auth"] = GSHEET_AUTH_TOKEN;
    payload["action"] = "info"; // Simple action to check connectivity
    // payload["spreadsheet_id"] = "..."; // Optional: Only if not hardcoded in Script

    // 2. Prepare Request
    std::string response_buffer;
    std::vector<std::string> headers = { "Content-Type: application/json" };
    
    // 3. Send POST Request
    // Note: We use payload.dump() to convert the JSON object to a std::string
    int http_code = httpPost(GSHEET_PRIV_API_URL, headers, payload.dump(), response_buffer);

    // 4. Output Result
    if (http_code == 200) {
        std::cout << "[Google Sheets] Success! Response:\n" << response_buffer << std::endl;
    } else {
        std::cerr << "[Google Sheets] Failed. HTTP Code: " << http_code << "\nResponse: " << response_buffer << std::endl;
    }
    // --- GOOGLE SHEETS API TEST END ---
    
    Bot scanManager(DISCORD_BOT_TOKEN, config);
    scanManager.start();
    
    return 0;
}
