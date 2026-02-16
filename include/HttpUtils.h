#pragma once

#include<string>
#include<vector>
#include <curl/curl.h>

class CurlGlobalManager {
public:
	CurlGlobalManager() {curl_global_init(CURL_GLOBAL_DEFAULT); }
	~CurlGlobalManager() {curl_global_cleanup();}

	CurlGlobalManager(const CurlGlobalManager&) = delete;
    CurlGlobalManager& operator=(const CurlGlobalManager&) = delete;
};

std::vector<std::string> urlEncode(std::vector<std::string>& unencoded_data);

int httpGet(const std::string& url, const std::vector<std::string>& headers, std::string& read_buffer);

int httpPost(const std::string& url, const std::vector<std::string>& headers, const std::string& post_data, std::string& read_buffer);
