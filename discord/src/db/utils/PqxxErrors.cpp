// Associated Header Include
#include "db/utils/PqxxErrors.hpp"

// Third Party Includes
#include <optional>
#include <pqxx/pqxx>
#include <string_view>

std::optional<std::string> Db::Utils::extractConstraintName(const pqxx::failure &e) {
    std::string_view msg = e.what();
    auto start = msg.find('"');
    auto end = msg.rfind('"');

    if (start == std::string_view::npos || end == start) {
        return std::nullopt;
    }

    return std::string(msg.substr(start + 1, end - start - 1));
}