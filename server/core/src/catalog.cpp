#include <fstream>
#include <functional>
#include <stdexcept>

#include "irods_configuration_keywords.hpp"
#include "irods_get_full_path_for_config_file.hpp"
#include "irods_logger.hpp"
#include "irods_stacktrace.hpp"
#include "objDesc.hpp"
#include "rodsConnect.h"

#include "catalog.hpp"

#include "json.hpp"
#include "fmt/format.h"
#include "nanodbc/nanodbc.h"

namespace irods::experimental::catalog {

    auto new_database_connection() -> std::tuple<std::string, nanodbc::connection>
    {
        using log       = irods::experimental::log;
        using json      = nlohmann::json;

        const std::string dsn = [] {
            if (const char* dsn = std::getenv("irodsOdbcDSN"); dsn) {
                return dsn;
            }

            return "iRODS Catalog";
        }();

        std::string config_path;

        if (const auto error = irods::get_full_path_for_config_file("server_config.json", config_path); !error.ok()) {
            log::database::error("Server configuration not found");
            throw std::runtime_error{"Failed to connect to catalog"};
        }

        log::database::trace("Reading server configuration ...");

        json config;

        {
            std::ifstream config_file{config_path};
            config_file >> config;
        }

        try {
            const auto& db_plugin_config = config.at(irods::CFG_PLUGIN_CONFIGURATION_KW).at(irods::PLUGIN_TYPE_DATABASE);
            const auto& db_instance = db_plugin_config.front();
            const auto db_username = db_instance.at(irods::CFG_DB_USERNAME_KW).get<std::string>();
            const auto db_password = db_instance.at(irods::CFG_DB_PASSWORD_KW).get<std::string>();

            // Capture the database instance name.
            std::string db_instance_name;
            for (auto& [k, v] : db_plugin_config.items()) {
                db_instance_name = k;
            }

            if (db_instance_name.empty()) {
                throw std::runtime_error{"Database instance name cannot be empty"};
            }

            log::database::trace(fmt::format("attempting connection to database using dsn [{}]", dsn));

            nanodbc::connection db_conn{dsn, db_username, db_password};

            if (db_instance_name == "mysql") {
                // MySQL must be running in ANSI mode (or at least in PIPES_AS_CONCAT mode) to be
                // able to understand Postgres SQL. STRICT_TRANS_TABLES must be set too, otherwise
                // inserting NULL into a "NOT NULL" column does not produce an error.
                nanodbc::just_execute(db_conn, "set SESSION sql_mode = 'ANSI,STRICT_TRANS_TABLES'");
                nanodbc::just_execute(db_conn, "set character_set_client = utf8");
                nanodbc::just_execute(db_conn, "set character_set_results = utf8");
                nanodbc::just_execute(db_conn, "set character_set_connection = utf8");
            }

            return {db_instance_name, db_conn};
        }
        catch (const std::exception& e) {
            log::database::error(e.what());
            throw std::runtime_error{"Failed to connect to catalog"};
        }
    } // new_database_connection

    auto execute_transaction(
        nanodbc::connection& _db_conn,
        std::function<int(nanodbc::transaction&)> _func) -> int
    {
        nanodbc::transaction trans{_db_conn};
        return _func(trans);
    } // execute_transaction

} // namespace irods::experimental::catalog
