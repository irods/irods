#ifndef IRODS_CATALOG_HPP
#define IRODS_CATALOG_HPP

#include "nanodbc/nanodbc.h"

#include <string>
#include <tuple>

namespace irods::experimental::catalog {

    auto new_database_connection() -> std::tuple<std::string, nanodbc::connection>;

    auto execute_transaction(
        nanodbc::connection& _db_conn,
        std::function<int(nanodbc::transaction&)> _func) -> int;

} // namespace irods::experimental::catalog

#endif // #ifndef IRODS_CATALOG_HPP
