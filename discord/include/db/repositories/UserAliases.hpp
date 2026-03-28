#pragma once

// Standard Includes
#include "db/ConnectionPool.hpp"
#include <string>

// Third Party Includes
#include <pqxx/pqxx>

class UserAliasesRepository {
public:
  void create(pqxx::work &txn, int user_id, std::string &alias);

  std::optional<std::string> read(pqxx::read_transaction &txn, int user_id);
  
  void retire(pqxx::work &txn, int user_id);
};