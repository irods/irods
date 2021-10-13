#include "catalog.hpp"
#include "irods_configuration_keywords.hpp"
#include "irods_get_full_path_for_config_file.hpp"
#include "irods_log.hpp"
#include "irods_server_properties.hpp"
#include "irods_stacktrace.hpp"
#include "objDesc.hpp"
#include "rodsConnect.h"

#include "json.hpp"
#include "fmt/format.h"
#include "nanodbc/nanodbc.h"

#include <fstream>
#include <functional>
#include <stdexcept>

namespace irods::experimental::catalog {

    auto new_database_connection(bool _read_server_config) -> std::tuple<std::string, nanodbc::connection>
    {
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
                rodsLog(LOG_ERROR, "Server configuration not found");
                throw std::runtime_error{"Failed to connect to catalog"};
            }

            rodsLog(LOG_DEBUG9, "Reading server configuration ...");

            nlohmann::json config;

            {
                std::ifstream config_file{config_path};
                config_file >> config;
            }

            const auto& db_plugin_config = config.at(irods::CFG_PLUGIN_CONFIGURATION_KW).at(irods::PLUGIN_TYPE_DATABASE);
            const auto& db_instance = db_plugin_config.front();

            db_instance_name = std::begin(db_plugin_config).key();
            db_username = db_instance.at(irods::CFG_DB_USERNAME_KW).get<std::string>();
            db_password = db_instance.at(irods::CFG_DB_PASSWORD_KW).get<std::string>();
        }
        else {
            // clang-format off
            using map_type      = std::unordered_map<std::string, boost::any>;
            using key_path_type = configuration_parser::key_path_t;
            // clang-format on

            const auto& db_plugin_config = get_server_property<const map_type&>(key_path_type{CFG_PLUGIN_CONFIGURATION_KW, PLUGIN_TYPE_DATABASE});
            const auto& [key, value] = *std::begin(db_plugin_config);
            const auto& db_instance = boost::any_cast<const map_type&>(value);

            db_instance_name = key;
            db_username = boost::any_cast<const std::string&>(db_instance.at(irods::CFG_DB_USERNAME_KW));
            db_password = boost::any_cast<const std::string&>(db_instance.at(irods::CFG_DB_PASSWORD_KW));
        }

        try {
            if (db_instance_name.empty()) {
                throw std::runtime_error{"Database instance name cannot be empty"};
            }

            rodsLog(LOG_DEBUG9, fmt::format("attempting connection to database using dsn [{}]", dsn).c_str());

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
            irods::log(LOG_ERROR, e.what());
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
