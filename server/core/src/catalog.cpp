#include "irods/catalog.hpp"
#include "irods/irods_configuration_keywords.hpp"
#include "irods/irods_get_full_path_for_config_file.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_server_properties.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/objDesc.hpp"
#include "irods/rodsConnect.h"

#include <nlohmann/json.hpp>
#include <fmt/format.h>
#include <nanodbc/nanodbc.h>

#include <fstream>
#include <functional>
#include <stdexcept>

using json = nlohmann::json;

namespace
{
    auto capture_database_info(const json& _server_config,
                               std::string& _db_instance_name,
                               std::string& _db_username,
                               std::string& _db_password) -> void
    {
        const auto& db_plugin_config = _server_config.at(irods::KW_CFG_PLUGIN_CONFIGURATION).at(irods::KW_CFG_PLUGIN_TYPE_DATABASE);
        const auto& db_instance = db_plugin_config.front();

        _db_instance_name = std::begin(db_plugin_config).key();
        _db_username = db_instance.at(irods::KW_CFG_DB_USERNAME).get<std::string>();
        _db_password = db_instance.at(irods::KW_CFG_DB_PASSWORD).get<std::string>();
    }
} // anonymous namespace

namespace irods::experimental::catalog {

    auto new_database_connection(bool _read_server_config) -> std::tuple<std::string, nanodbc::connection>
    {
        namespace log = irods::experimental::log;

        const std::string dsn = [] {
            if (const char* dsn = std::getenv("irodsOdbcDSN"); dsn) {
                return dsn;
            }

            return "iRODS Catalog";
        }();

        std::string db_username;
        std::string db_password;
        std::string db_instance_name;

        if (_read_server_config) {
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

            capture_database_info(config, db_instance_name, db_username, db_password);
        }
        else {
            const auto config_handle{server_properties::instance().map()};
            const auto& config{config_handle.get_json()};
            capture_database_info(config, db_instance_name, db_username, db_password);
        }

        try {
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
