// Associated Header Include
#include "db/DbSession.hpp"

// User Defined Includes
#include "db/ConnectionPool.hpp"

DbSession::DbSession(ConnectionPool &pool)
    : m_pool(pool),
      m_conn(pool.acquire()),
      m_txn(*m_conn) {}

DbSession::~DbSession() {
  m_pool.release(std::move(m_conn));
}

pqxx::work &DbSession::tx() {
  return m_txn;
}

void DbSession::commit() {
  m_txn.commit();
}