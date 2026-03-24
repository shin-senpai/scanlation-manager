#pragma once

// Standard Includes
#include <optional>
#include <string>

// Third Party Includes
#include <pqxx/pqxx>

namespace Db::Utils {
// Extracts the constraint name from a pqxx unique_violation exception.
// Returns nullopt if the message format is unexpected.
std::optional<std::string> extractConstraintName(const pqxx::failure &e);
}