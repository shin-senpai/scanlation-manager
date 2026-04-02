// Associated Header Include
#include "db/DbSession.hpp"

// User Defined Includes
#include "db/ConnectionPool.hpp"

DbSession::DbSession(ConnectionPool &pool)
    : m_pool(pool),
      m_conn(pool.acquire()) {}

DbSession::~DbSession() {
  m_pool.release(std::move(m_conn));
}

pqxx::work &DbSession::wtx() {
  if(m_rtxn) {
    throw std::logic_error("You are trying to use a write_tx when a read_tx is already active");
  }
  if(!m_wtxn) {
    m_wtxn.emplace(*m_conn);
  }
  return *m_wtxn;
}

pqxx::read_transaction &DbSession::rtx() {
  if(m_wtxn) {
    throw std::logic_error("You are trying to use a read_tx when a write_tx is already active");
  }
  if(!m_rtxn) {
    m_rtxn.emplace(*m_conn);
  }
  return *m_rtxn;
}

void DbSession::closeTx() {
  if(m_rtxn) {
    m_rtxn.reset();
  }
  if(m_wtxn) {
    m_wtxn.reset();
  }
}

void DbSession::commit() {
  if(!m_wtxn) {
    throw std::logic_error("write tx not active");
  }
  m_wtxn->commit();

  m_wtxn.reset();
}