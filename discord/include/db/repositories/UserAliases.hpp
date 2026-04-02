#pragma once

// Standard Includes
#include "db/ConnectionPool.hpp"
#include <string>

// Third Party Includes
#include <pqxx/pqxx>

class UserAliasesRepository {
public:
  void create(pqxx::transaction_base &txn, int user_id, std::string &alias);

  std::optional<std::string> read(pqxx::transaction_base &txn, int user_id);
  
  void retire(pqxx::transaction_base &txn, int user_id);
};