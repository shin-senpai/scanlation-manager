// Associated Header Include
#include "db/ConnectionPool.hpp"

// User Defined Includes

// Standard Includes
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <utility>

// Third Party Includes
#include <pqxx/pqxx>

ConnectionPool::ConnectionPool(const std::string &connStr, size_t size) {
  for(size_t i = 0; i < size; ++i) {
    m_pool.push(std::make_unique<pqxx::connection>(connStr));
  }
}

std::unique_ptr<pqxx::connection> ConnectionPool::acquire() {
  std::unique_lock lock(m_mutex);

  m_cv.wait(lock, [&] { return !m_pool.empty(); });

  auto conn = std::move(m_pool.front());
  m_pool.pop();
  return conn;
}

void ConnectionPool::release(std::unique_ptr<pqxx::connection> conn) {
  std::lock_guard lock(m_mutex);
  m_pool.push(std::move(conn));
  m_cv.notify_one();
}