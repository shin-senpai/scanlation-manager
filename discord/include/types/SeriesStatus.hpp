#pragma once

// Standard Includes
#include <stdexcept>
#include <string>

enum class SeriesStatus { active, completed, dropped, hiatus };

inline SeriesStatus seriesStatusFromString(const std::string &s) {
  if(s == "active") return SeriesStatus::active;
  if(s == "completed") return SeriesStatus::completed;
  if(s == "dropped") return SeriesStatus::dropped;
  if(s == "hiatus") return SeriesStatus::hiatus;
  throw std::invalid_argument("Unknown series status: " + s);
}

inline std::string seriesStatusToString(SeriesStatus s) {
  switch(s) {
    case SeriesStatus::active: return "active";
    case SeriesStatus::completed: return "completed";
    case SeriesStatus::dropped: return "dropped";
    case SeriesStatus::hiatus: return "hiatus";
  }
}
