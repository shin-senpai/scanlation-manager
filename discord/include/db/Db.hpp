#pragma once

// User Defined Includes
#include "db/ConnectionPool.hpp"
#include "db/DbSession.hpp"

// Standard Includes
#include <cstddef>
#include <string>

class Db {
private:
  ConnectionPool m_pool;

public:
  Db(const std::string &connStr, size_t poolSize);

  Db(const Db &) = delete;
  Db &operator=(const Db &) = delete;

  DbSession session();
};