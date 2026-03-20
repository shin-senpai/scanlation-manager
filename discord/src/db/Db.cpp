// Associated Header Include
#include "db/Db.hpp"

Db::Db(const std::string &connStr, size_t poolSize)
    : m_pool(connStr, poolSize) {}

DbSession Db::session() {
  return DbSession(m_pool);
}