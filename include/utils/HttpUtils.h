#pragma once

// Standard Includes
#include <string>
#include <vector>

// Third Party Includes

class CurlGlobalManager {

private:
  CurlGlobalManager();

public:
  static void curlManagerInit();

  ~CurlGlobalManager();

  CurlGlobalManager(const CurlGlobalManager &) = delete;
  CurlGlobalManager &operator=(const CurlGlobalManager &) = delete;
  CurlGlobalManager(CurlGlobalManager &&) = delete;
  CurlGlobalManager &operator=(CurlGlobalManager &&) = delete;
};

std::vector<std::string>
urlEncode(const std::vector<std::string> &unencoded_data);

int httpGet(const std::string &url, const std::vector<std::string> &headers,
            std::string &read_buffer);

int httpPost(const std::string &url, const std::vector<std::string> &headers,
             const std::string &post_data, std::string &read_buffer);
