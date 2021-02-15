#include "irods_server_properties.hpp"

#include "irods_get_full_path_for_config_file.hpp"
#include "rodsLog.h"

#include <boost/any.hpp>
#include <json.hpp>

#include <fstream>
#include <unordered_map>

namespace irods
{
    const std::string STRICT_ACL_KW( "strict_acls" );
    const std::string AGENT_CONN_KW( "agent_conn" );
    const std::string AGENT_KEY_KW( "agent_key" );
    const std::string RE_PID_KW( "rule_engine_process_id" );
    const std::string CLIENT_USER_NAME_KW( "client_user_name" );
    const std::string CLIENT_USER_ZONE_KW( "client_user_zone" );
    const std::string CLIENT_USER_PRIV_KW( "client_user_priv" );
    const std::string PROXY_USER_NAME_KW( "proxy_user_name" );
    const std::string PROXY_USER_ZONE_KW( "proxy_user_zone" );
    const std::string PROXY_USER_PRIV_KW( "proxy_user_priv" );
    const std::string SERVER_CONFIG_FILE( "server_config.json" );

    server_properties& server_properties::instance()
    {
        static server_properties singleton;
        return singleton;
    } // instance

    server_properties::server_properties()
    {
        capture();
    } // ctor

    void server_properties::capture()
    {
        // if a json version exists, then attempt to capture that
        std::string svr_cfg;
        irods::error ret = irods::get_full_path_for_config_file( SERVER_CONFIG_FILE, svr_cfg );
        capture_json( svr_cfg );

        std::string db_cfg;
        ret = irods::get_full_path_for_config_file( "database_config.json", db_cfg );
        if ( ret.ok() ) {
            capture_json( db_cfg );
        }
    } // capture

    void server_properties::capture_json( const std::string& _filename )
    {
        error ret = config_props_.load( _filename );
        if ( !ret.ok() ) {
            THROW( ret.code(), ret.result() );
        }
    } // capture_json

    void server_properties::remove( const std::string& _key )
    {
        config_props_.remove( _key );
    } // remove

    void delete_server_property( const std::string& _prop )
    {
        irods::server_properties::instance().remove(_prop);
    } // delete_server_property

    auto get_dns_cache_shared_memory_size() noexcept -> int
    {
        try {
            using map_type = std::unordered_map<std::string, boost::any>;
            const auto wrapped = get_advanced_setting<map_type&>(CFG_DNS_CACHE_KW).at(CFG_SHARED_MEMORY_SIZE_IN_BYTES_KW);
            const auto bytes = boost::any_cast<int>(wrapped);

            if (bytes > 0) {
                return bytes;
            }

            rodsLog(LOG_ERROR, "Invalid shared memory size for DNS cache [size=%d].", bytes);
        }
        catch (...) {
            rodsLog(LOG_DEBUG, "Could not read server configuration property [%s.%s.%s].",
                    CFG_ADVANCED_SETTINGS_KW.data(), CFG_DNS_CACHE_KW.data(), CFG_SHARED_MEMORY_SIZE_IN_BYTES_KW.data());
        }

        rodsLog(LOG_DEBUG, "Returning default shared memory size for DNS cache [default=5000000].");

        return 5'000'000;
    } // get_dns_cache_shared_memory_size

    int get_dns_cache_eviction_age() noexcept
    {
        try {
            using map_type = std::unordered_map<std::string, boost::any>;
            const auto wrapped = get_advanced_setting<map_type&>(CFG_DNS_CACHE_KW).at(CFG_EVICTION_AGE_IN_SECONDS_KW);
            const auto seconds =  boost::any_cast<int>(wrapped);

            if (seconds >= 0) {
                return seconds;
            }

            rodsLog(LOG_ERROR, "Invalid eviction age for DNS cache [seconds=%d].", seconds);
        }
        catch (...) {
            rodsLog(LOG_DEBUG, "Could not read server configuration property [%s.%s.%s].",
                    CFG_ADVANCED_SETTINGS_KW.data(), CFG_DNS_CACHE_KW.data(), CFG_EVICTION_AGE_IN_SECONDS_KW.data());
        }

        rodsLog(LOG_DEBUG, "Returning default eviction age for DNS cache [default=3600].");

        return 3600;
    } // get_dns_cache_eviction_age

    auto get_hostname_cache_shared_memory_size() noexcept -> int
    {
        try {
            using map_type = std::unordered_map<std::string, boost::any>;
            const auto wrapped = get_advanced_setting<map_type&>(CFG_HOSTNAME_CACHE_KW).at(CFG_SHARED_MEMORY_SIZE_IN_BYTES_KW);
            const auto bytes = boost::any_cast<int>(wrapped);

            if (bytes > 0) {
                return bytes;
            }

            rodsLog(LOG_ERROR, "Invalid shared memory size for Hostname cache [size=%d].", bytes);
        }
        catch (...) {
            rodsLog(LOG_DEBUG, "Could not read server configuration property [%s.%s.%s].",
                    CFG_ADVANCED_SETTINGS_KW.data(), CFG_HOSTNAME_CACHE_KW.data(), CFG_SHARED_MEMORY_SIZE_IN_BYTES_KW.data());
        }

        rodsLog(LOG_DEBUG, "Returning default shared memory size for Hostname cache [default=2500000].");

        return 2'500'000;
    } // get_hostname_cache_shared_memory_size

    int get_hostname_cache_eviction_age() noexcept
    {
        try {
            using map_type = std::unordered_map<std::string, boost::any>;
            const auto wrapped = get_advanced_setting<map_type&>(CFG_HOSTNAME_CACHE_KW).at(CFG_EVICTION_AGE_IN_SECONDS_KW);
            const auto seconds =  boost::any_cast<int>(wrapped);

            if (seconds >= 0) {
                return seconds;
            }

            rodsLog(LOG_ERROR, "Invalid eviction age for Hostname cache [seconds=%d].", seconds);
        }
        catch (...) {
            rodsLog(LOG_DEBUG, "Could not read server configuration property [%s.%s.%s].",
                    CFG_ADVANCED_SETTINGS_KW.data(), CFG_HOSTNAME_CACHE_KW.data(), CFG_EVICTION_AGE_IN_SECONDS_KW.data());
        }

        rodsLog(LOG_DEBUG, "Returning default eviction age for Hostname cache [default=3600].");

        return 3600;
    } // get_hostname_cache_eviction_age

    void parse_and_store_hosts_configuration_file_as_json() noexcept
    {
        try {
            std::string path;

            if (const auto error = get_full_path_for_config_file("hosts_config.json", path); !error.ok()) {
                if (error.code() != SYS_INVALID_INPUT_PARAM) {
                    rodsLog(LOG_ERROR, "Could not read file [%s].", path.data());
                }

                return;
            }

            const auto hosts_config = nlohmann::json::parse(std::ifstream{path});
            irods::set_server_property(irods::HOSTS_CONFIG_JSON_OBJECT_KW, hosts_config);
        }
        catch (...) {
            rodsLog(LOG_ERROR, "An unexpected error occurred while processing the hosts_config.json file.");
        }
    } // parse_and_store_hosts_configuration_file_as_json
} // namespace irods

