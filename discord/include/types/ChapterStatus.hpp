#pragma once

// Standard Includes
#include <stdexcept>
#include <string>

enum class ChapterStatus { in_progress, queued, released, dropped, hiatus };

inline ChapterStatus chapterStatusFromString(const std::string &s) {
  if(s == "in_progress") {
    return ChapterStatus::in_progress;
}
  if(s == "queued") {
    return ChapterStatus::queued;
}
  if(s == "released") {
    return ChapterStatus::released;
}
  if(s == "dropped") {
    return ChapterStatus::dropped;
}
  if(s == "hiatus") {
    return ChapterStatus::hiatus;
}
  throw std::invalid_argument("Unknown chapter status: " + s);
}

inline std::string chapterStatusToString(ChapterStatus s) {
  switch(s) {
    case ChapterStatus::in_progress:
      return "in_progress";
    case ChapterStatus::queued:
      return "queued";
    case ChapterStatus::released:
      return "released";
    case ChapterStatus::dropped:
      return "dropped";
    case ChapterStatus::hiatus:
      return "hiatus";
  }
}
