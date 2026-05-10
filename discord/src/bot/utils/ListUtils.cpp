// Associated Header Include
#include "bot/utils/ListUtils.hpp"

// Standard Includes
#include <algorithm>
#include <string>
#include <vector>

std::vector<std::string> BotUtils::makePages(
    const std::string &title,
    const std::vector<std::string> &lines,
    size_t per_page) {
  if(lines.empty()) {
    return {title + "\nNo results found."};
  }

  const size_t total = (lines.size() + per_page - 1) / per_page;
  std::vector<std::string> pages;
  pages.reserve(total);

  for(size_t i = 0; i < lines.size(); i += per_page) {
    std::string page = title + " [Page " + std::to_string(i / per_page + 1) + "/" + std::to_string(total) + "]\n";
    const size_t end = std::min(i + per_page, lines.size());
    for(size_t j = i; j < end; ++j) {
      page += lines[j] + "\n";
    }
    pages.push_back(std::move(page));
  }

  return pages;
}
