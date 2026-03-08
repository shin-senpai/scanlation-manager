// Associated Header Include
#include "utils/HttpUtils.hpp"

// User Defined Includes

// Standard Includes
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// Third Party Includes
#include <curl/curl.h>

CurlGlobalManager::CurlGlobalManager() {
  curl_global_init(CURL_GLOBAL_DEFAULT);
}

CurlGlobalManager::~CurlGlobalManager() { curl_global_cleanup(); }

void CurlGlobalManager::curlManagerInit() { static CurlGlobalManager instance; }

class CurlWrapper {
private:
  CURL *m_curl;
  struct ::curl_slist *m_headers;

public:
  CurlWrapper() : m_curl(curl_easy_init()), m_headers(nullptr) {
    if(!m_curl) {
      throw std::runtime_error("Unable to initialize CURL");
    }
  }

  ~CurlWrapper() {
    if(m_headers) {
      curl_slist_free_all(m_headers);
    }
    if(m_curl) {
      curl_easy_cleanup(m_curl);
    }
  }

  CURL *getCurl() { return m_curl; }

  void appendHeaders(const std::vector<std::string> &header_list) {
    if(m_headers) {
      curl_slist_free_all(m_headers);
      m_headers = nullptr;
    }
    for(const auto &header_item : header_list) {
      m_headers = curl_slist_append(m_headers, header_item.c_str());
    }
  }

  struct ::curl_slist *getHeaders() { return m_headers; }

  std::string encode(const std::string &unencoded_element) {
    char *encode =
        curl_easy_escape(m_curl, unencoded_element.c_str(),
                         static_cast<int>(unencoded_element.length()));
    if(!encode) {
      throw std::runtime_error("Failed to encode string");
    }
    std::string encoded_str{static_cast<std::string>(encode)};
    curl_free(encode);
    return encoded_str;
  }

  CurlWrapper operator=(const CurlWrapper &) = delete;
  CurlWrapper(const CurlWrapper &) = delete;
};

static size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  (static_cast<std::string *>(static_cast<void *>(userp)))
      ->append(static_cast<char *>(static_cast<void *>(contents)),
               size * nmemb);
  return size * nmemb;
}

// URL Encodes all elements of a passed vector
std::vector<std::string> urlEncode(const std::vector<std::string> &unencoded_data) {
  CurlWrapper wrapper;
  std::vector<std::string> encoded_data{};
  for(const std::string &str : unencoded_data) {
    encoded_data.push_back(wrapper.encode(str));
  }
  return encoded_data;
}

static int executeRequest(CurlWrapper &wrapper, const std::string &url,
                          std::string &read_buffer) {
  // curl_easy_setopt(wrapper.getCurl(), CURLOPT_VERBOSE, 1L); - Uncomment this for verbose logs
  curl_easy_setopt(wrapper.getCurl(), CURLOPT_URL, url.c_str());
  curl_easy_setopt(wrapper.getCurl(), CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(wrapper.getCurl(), CURLOPT_USERAGENT, "scanlation-manager/0.0.1");
  curl_easy_setopt(wrapper.getCurl(), CURLOPT_HTTPHEADER, wrapper.getHeaders());
  curl_easy_setopt(wrapper.getCurl(), CURLOPT_WRITEFUNCTION, writeCallback);
  curl_easy_setopt(wrapper.getCurl(), CURLOPT_WRITEDATA, &read_buffer);

  CURLcode response = curl_easy_perform(wrapper.getCurl());
  if(response != CURLE_OK) {
    std::cerr << "Request failed: " << curl_easy_strerror(response) << '\n';
    return 0;
  }

  int http_code{};
  curl_easy_getinfo(wrapper.getCurl(), CURLINFO_RESPONSE_CODE, &http_code);

  return http_code;
}

int httpGet(const std::string &url, const std::vector<std::string> &headers,
            std::string &read_buffer) {

  CurlWrapper wrapper;
  wrapper.appendHeaders(headers);

  return executeRequest(wrapper, url, read_buffer);
}

int httpPost(const std::string &url, const std::vector<std::string> &headers,
             const std::string &post_data, std::string &read_buffer) {

  CurlWrapper wrapper;
  wrapper.appendHeaders(headers);

  curl_easy_setopt(wrapper.getCurl(), CURLOPT_POST, 1L);
  curl_easy_setopt(wrapper.getCurl(), CURLOPT_POSTFIELDS, post_data.c_str());
  curl_easy_setopt(wrapper.getCurl(), CURLOPT_POSTFIELDSIZE, static_cast<long>(post_data.length()));

  return executeRequest(wrapper, url, read_buffer);
}