// Associated Header Include

// User Defined Includes
#include "discord/DiscordUtils.h"

// Standard Includes
#include <string_view>

// Third Party Includes
#include <dpp/dispatcher.h>

struct WorkUpdate {
  std::string staff_name;
  std::string series_channel;
  std::string chapter;
  std::string task;
  std::string next_role;
};

static void normalize(std::string &str) {
  auto new_end = std::remove_if(
      str.begin(), str.end(), [](unsigned char x) { return std::isspace(x); });
  str.erase(new_end, str.end());
}

void Bot::triggerWorkUpdate(const dpp::message_create_t &event) {
  std::string content = event.msg.content;
  normalize(content);

    

  if (std::count(content.begin(), content.end(), '|') < 3)
    return;

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
      update.next_role = segment;
      break;
    }
    if (end == std::string_view::npos)
      break;
    start = end + 1;
  }

  std::string response = "Work Update\nStaff Name: " + update.staff_name +
                         "\nSeries: " + update.series_channel +
                         "\nChapter: " + update.chapter +
                         "\nTask: " + update.task +
                         "\nNext Role: " + update.next_role;

  event.reply(response);

  // std::stringstream ss(content);
  // std::string segment;
  // std::vector<std::string> parts;

  // while(std::getline(ss, segment, '|')) {
  //     parts.push_back(segment);
  // }

  // if(parts.size() == 4){
  //     update.staff_name = parts[0];
  //     update.series_channel = parts[1];
  //     update.chapter = parts[2];
  //     update.next_role = parts[3];
  // }
}