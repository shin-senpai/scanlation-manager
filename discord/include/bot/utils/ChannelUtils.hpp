#pragma once

// User Defined Includes

// Standard Includes
#include <optional>
#include <string_view>

// Third Party Includes
#include <dpp/snowflake.h>

namespace BotUtils {
bool extractChannelId(std::string_view &sv, dpp::snowflake &channel_id);
std::optional<std::string> extractChannelName(std::string_view sv);
}