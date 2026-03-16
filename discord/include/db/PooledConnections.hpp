#pragma once

// User Defined Includes
#include "db/ConnectionPool.hpp"

// Standard Includes
#include <memory>

// Third Party Includes
#include <pqxx/pqxx>

class PooledConnections {
private:
  ConnectionPool &m_pool;
  std::unique_ptr<pqxx::connection> m_conn;

public:
  explicit PooledConnections(ConnectionPool &pool);

  ~PooledConnections();

  pqxx::connection &get();
};