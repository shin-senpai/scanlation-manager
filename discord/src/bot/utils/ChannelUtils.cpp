// Associated Header Include
#include "bot/utils/ChannelUtils.hpp"

// User Defined Includes

// Standard Includes
#include <charconv>
#include <cstdint>
#include <optional>
#include <string_view>
#include <system_error>

// Third Party Includes
#include <dpp/cache.h>
#include <dpp/channel.h>
#include <dpp/snowflake.h>

// Extract channel id channel mentions. Only supports mentions aka <#[snowflake]>
bool BotUtils::extractChannelId(std::string_view &sv, dpp::snowflake &channel_id) {

  size_t start = sv.find("<#");
  if(start == std::string_view::npos) {
    return false;
  }

  size_t end = sv.find('>', start);
  if(end == std::string_view::npos) {
    return false;
  }

  start += 2; // skips "<#"
  std::string_view id_str = sv.substr(start, end - start);

  uint64_t val = 0;
  auto [ptr, ec] = std::from_chars(id_str.data(), id_str.data() + id_str.size(), val);

  if(ec == std::errc() && ptr == id_str.data() + id_str.size()) {
    channel_id = val;
    return true;
  }

  return false;
}

std::optional<std::string> BotUtils::extractChannelName(std::string_view sv) {

  dpp::snowflake channel_id;
  if(!BotUtils::extractChannelId(sv, channel_id)) {
    return std::nullopt;
  }

  dpp::channel *channel = dpp::find_channel(channel_id);
  if(channel != nullptr) {
    return channel->name;
  }

  return std::nullopt;
}