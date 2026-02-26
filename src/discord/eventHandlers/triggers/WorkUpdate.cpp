// Associated Header Include

// User Defined Includes
#include "discord/Bot.h"

// Standard Includes
#include <cstddef>
#include <cstdint>
#include <dpp/snowflake.h>
#include <string>
#include <string_view>
#include <charconv>

// Third Party Includes
#include <dpp/dispatcher.h>

struct WorkUpdate {
  std::string staff_name;
  std::string series_channel;
  std::string series_channel_name;
  std::string chapter;
  std::string task;
  std::string next_role = "";
};

static void normalize(std::string &str) {
  auto new_end = std::remove_if(
      str.begin(), str.end(), [](unsigned char x) { return std::isspace(x) || x < 32 || x > 126; });
  str.erase(new_end, str.end());
}

//Extract channel id channel mentions. Only supports mentions aka <#[snowflake]>
static bool extractChannelId(std::string_view &sv, dpp::snowflake &channel_id) {

  size_t start = sv.find("<#");
  if (start == std::string_view::npos) {
    return false;
  }

  size_t end = sv.find('>', start);
  if (end == std::string_view::npos) {
    return false;
  }

  // The actual ID starts 2 characters after the '<'
  start += 2;
  std::string_view id_str = sv.substr(start, end - start);

  uint64_t val = 0;
  auto [ptr, ec] = std::from_chars(id_str.data(), id_str.data() + id_str.size(), val);

  if (ec == std::errc()) {
       channel_id = val;
       return true;
  }

  return false;
}

static bool extractChannelName(std::string_view sv, WorkUpdate &update){
  
  dpp::snowflake channel_id;
  if(!extractChannelId(sv, channel_id)){
    return false;
  }

  dpp::channel* channel = dpp::find_channel(channel_id);
  if(channel != nullptr) {
    update.series_channel_name = channel->name;
    return true;
  }

  return false;
}

void Bot::triggerWorkUpdate(const dpp::message_create_t &event) {
  std::string content = event.msg.content;
  normalize(content);
  
  bool has_next_role;
  if (std::count(content.begin(), content.end(), '|') == 4) {
    has_next_role = true;
  }
  else if (std::count(content.begin(), content.end(), '|') == 3) {
    has_next_role = false;
  }
  else{
    return;
  }

  WorkUpdate update;
  std::string_view sv(content);
  size_t start = 0;
  size_t end = 0;

  for (int i = 0; i < 5; ++i) {
    end = sv.find('|', start);

    std::string_view segment = sv.substr(start, end - start);

    switch (i) {
    case 0:
      update.staff_name = segment;
      break;
    case 1:
      update.series_channel = segment;
      break;
    case 2:
      update.chapter = segment;
      break;
    case 3:
      update.task = segment;
      break;
    case 4:
      if(has_next_role) {
        update.next_role = segment;
      }
      break;
    }

    if (end == std::string_view::npos) break;

    start = end + 1;
  }

  if(!extractChannelName(update.series_channel, update)) {
    update.series_channel_name = "";
  }

  std::string response = "Work Update\nStaff Name: " + update.staff_name +
                         "\nSeries Channel: " + update.series_channel +
                         "\nSeries Channel Name: " + update.series_channel_name +
                         "\nChapter: " + update.chapter +
                         "\nTask: " + update.task +
                         "\nNext Role: " + update.next_role;

  event.reply(response);
}