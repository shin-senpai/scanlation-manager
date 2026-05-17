// Associated Header Include
#include "bot/utils/SheetSync.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/BotSettings.hpp"
#include "utils/HttpUtils.hpp"

// Standard Includes
#include <iostream>
#include <string>
#include <thread>

namespace {

struct BackendConfig {
  std::string url;
  std::string token;
};

std::optional<BackendConfig> loadConfig(Bot &bot) {
  if(bot.getBackendUrl().empty() || bot.getApiToken().empty()) {
    return std::nullopt;
  }
  try {
    DbSession session(bot.getPool());
    BotSettingsRepository settings_repo;
    if(!settings_repo.get(session.rtx(), "gsheet_enabled")) {
      return std::nullopt;
    }
    return BackendConfig{bot.getBackendUrl(), bot.getApiToken()};
  } catch(const std::exception &e) {
    std::cerr << "SheetSync: failed to load config: " << e.what() << std::endl;
    return std::nullopt;
  }
}

void fireAndForget(const std::string &url, const std::string &token, const std::string &endpoint, const std::string &body) {
  std::thread([url, token, endpoint, body]() {
    try {
      std::string read_buffer;
      const int status = httpPost(url + endpoint, {"Authorization: Bearer " + token, "Content-Type: application/json"}, body, read_buffer);
      if(status != 200) {
        std::cerr << "SheetSync: POST " << endpoint << " returned HTTP " << status << std::endl;
      }
    } catch(const std::exception &e) {
      std::cerr << "SheetSync: POST " << endpoint << " failed: " << e.what() << std::endl;
    }
  }).detach();
}

} // namespace

void SheetSync::syncSeries(Bot &bot, const std::string &series_name) {
  const auto cfg = loadConfig(bot);
  if(!cfg) {
    return;
  }
  fireAndForget(cfg->url, cfg->token, "/sheets/sync/series", "{\"name\":\"" + series_name + "\"}");
}

void SheetSync::syncTodo(Bot &bot) {
  const auto cfg = loadConfig(bot);
  if(!cfg) {
    return;
  }
  fireAndForget(cfg->url, cfg->token, "/sheets/sync/todo", "{}");
}

void SheetSync::deleteSeries(Bot &bot, const std::string &series_name) {
  const auto cfg = loadConfig(bot);
  if(!cfg) {
    return;
  }
  fireAndForget(cfg->url, cfg->token, "/sheets/delete-series", "{\"name\":\"" + series_name + "\"}");
}
