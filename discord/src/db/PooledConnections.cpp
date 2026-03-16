// Associated Header Include
#include "db/PooledConnections.hpp"

// User Defined Includes
#include "db/ConnectionPool.hpp"

// Standard Includes
#include <memory>
#include <utility>

// Third Party Includes
#include <pqxx/pqxx>

PooledConnections::PooledConnections(ConnectionPool &pool) : m_pool(pool), m_conn(pool.acquire()){};

PooledConnections::~PooledConnections() {
  m_pool.release(std::move(m_conn));
}

pqxx::connection &PooledConnections::get() {
  return *m_conn;
}