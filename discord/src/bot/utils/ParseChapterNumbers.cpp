// Associated Header Include
#include "bot/utils/ParseChapterNumbers.hpp"

// Standard Includes
#include <algorithm>
#include <sstream>

std::vector<double> BotUtils::parseChapterNumbers(const std::string &input, std::string &error) {
  std::vector<double> result;
  std::istringstream ss(input);
  std::string token;
  while(std::getline(ss, token, ',')) {
    const auto start = token.find_first_not_of(" \t\r\n");
    const auto end = token.find_last_not_of(" \t\r\n");
    if(start == std::string::npos) {
      continue;
    }
    token = token.substr(start, end - start + 1);
    try {
      size_t pos;
      const double n = std::stod(token, &pos);
      if(pos != token.size()) {
        error = "Invalid chapter number: `" + token + "`";
        return {};
      }
      if(n < 0) {
        error = "Chapter number cannot be negative: `" + token + "`";
        return {};
      }
      if(std::find(result.begin(), result.end(), n) == result.end()) {
        result.push_back(n);
      }
    } catch(const std::exception &) {
      error = "Invalid chapter number: `" + token + "`";
      return {};
    }
  }
  if(result.empty()) {
    error = "No valid chapter numbers found.";
    return {};
  }
  return result;
}
