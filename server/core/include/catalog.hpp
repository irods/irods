#ifndef IRODS_CATALOG_HPP
#define IRODS_CATALOG_HPP

#include "nanodbc/nanodbc.h"

#include <string>
#include <tuple>

namespace irods::experimental::catalog
{
    /// \brief Establish connection to database hosting the catalog
    ///
    /// \param[in] _read_server_config Instructs the function to read and parse server_config.json.
    ///                                Defaults to true.
    ///
    /// \returns Tuple of the database type (string) and the database connection
    ///
    /// \throws std::runtime_error If any of the following conditions are met:
    /// - server_config.json cannot be found.
    /// - the database instance name is empty.
    /// - an exception is thrown.
    ///
    /// \since 4.2.9
    auto new_database_connection(bool _read_server_config = true)
        -> std::tuple<std::string, nanodbc::connection>;

    /// \brief Provides a transaction to the provided function and executes it
    ///
    /// \param[in] _db_conn Established connection to the database
    /// \param[in] _func    Function to execute using the generated transaction
    ///
    /// \returns Error code resulting from the executed transaction
    ///
    /// \since 4.2.9
    auto execute_transaction(
        nanodbc::connection& _db_conn,
        std::function<int(nanodbc::transaction&)> _func) -> int;
} // namespace irods::experimental::catalog

#endif // #ifndef IRODS_CATALOG_HPP
