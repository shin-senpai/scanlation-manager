#pragma once

// Standard Includes
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

// Third Party Includes
#include <pqxx/pqxx>

class ConnectionPool {
private:
  std::queue<std::unique_ptr<pqxx::connection>> m_pool;
  std::mutex m_mutex;
  std::condition_variable m_cv;

public:
  ConnectionPool(const std::string &connStr, size_t size);

  std::unique_ptr<pqxx::connection> acquire();

  void release(std::unique_ptr<pqxx::connection> conn);
};