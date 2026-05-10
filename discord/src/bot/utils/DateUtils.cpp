// Associated Header Include
#include "bot/utils/DateUtils.hpp"

// Standard Includes
#include <ctime>
#include <string>

std::string BotUtils::toDiscordTimestamp(const std::string &date) {
  std::tm tm{};
  strptime(date.c_str(), "%Y-%m-%d", &tm);
  const time_t t = timegm(&tm);
  return "<t:" + std::to_string(static_cast<long long>(t)) + ":D>";
}
