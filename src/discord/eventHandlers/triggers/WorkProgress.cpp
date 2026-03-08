// Associated Header Include
#include "discord/eventHandlers/triggers/WorkProgress.hpp"

// User Defined Includes
#include "discord/utils/ChannelUtils.hpp"
#include "models/ModelWorkProgress.hpp"

// Standard Includes
#include <cstddef>
#include <dpp/snowflake.h>
#include <string>
#include <string_view>

// Third Party Includes
#include <dpp/cache.h>
#include <dpp/dispatcher.h>

static void normalize(std::string &str) {
  auto new_end = std::remove_if(str.begin(), str.end(), [](unsigned char x) {
    return std::isspace(x) || x < 32 || x > 126;
  });
  str.erase(new_end, str.end());
}

void Triggers::workProgress(const dpp::message_create_t &event) {
  std::string content = event.msg.content;
  normalize(content);

  bool has_next_role;
  if(std::count(content.begin(), content.end(), '|') == 4) {
    has_next_role = true;
  } else if(std::count(content.begin(), content.end(), '|') == 3) {
    has_next_role = false;
  } else {
    return;
  }

  WorkProgress update;
  std::string_view sv(content);
  size_t start = 0;
  size_t end = 0;

  for(int i = 0; i < 5; ++i) {
    end = sv.find('|', start);

    std::string_view segment = sv.substr(start, end - start);

    switch(i) {
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

    if(end == std::string_view::npos)
      break;

    start = end + 1;
  }

  auto name = ChannelUtils::extractChannelName(update.series_channel);
  update.series_channel_name = name.value_or("");

  std::string response =
      "Work Update\nStaff Name: " + update.staff_name +
      "\nSeries Channel: " + update.series_channel +
      "\nSeries Channel Name: " + update.series_channel_name +
      "\nChapter: " + update.chapter + "\nTask: " + update.task +
      "\nNext Role: " + update.next_role;

  event.reply(response);
}