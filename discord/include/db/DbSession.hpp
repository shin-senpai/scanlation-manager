#pragma once

// User Defined Includes
#include "db/ConnectionPool.hpp"

// Standard Includes
#include <memory>

// Third Party Includes
#include <pqxx/pqxx>

class DbSession {
private:
  ConnectionPool &m_pool;
  std::unique_ptr<pqxx::connection> m_conn;
  pqxx::work m_txn;

public:
  explicit DbSession(ConnectionPool &pool);
  ~DbSession();

  DbSession(const DbSession &) = delete;
  DbSession &operator=(const DbSession &) = delete;

  pqxx::work &tx();
  void commit();
};