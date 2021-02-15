#ifndef IRODS_SERVER_PROPERTIES_HPP_
#define IRODS_SERVER_PROPERTIES_HPP_

/// \file

#include "irods_configuration_parser.hpp"
#include "irods_configuration_keywords.hpp"
#include "irods_error.hpp"
#include "irods_exception.hpp"

#include <string>

namespace irods
{
    /// @brief kw for server property map storing strict acl configuration
    extern const std::string STRICT_ACL_KW;

    /// @brief kw for server property map stating this is an agent-agent conn
    extern const std::string AGENT_CONN_KW;

    /// @brief kw for server property map for encryption key
    extern const std::string AGENT_KEY_KW;

    /// @brief kw for storing the process id of the rule engine server
    extern const std::string RE_PID_KW;

    /// @brief kw for storing client user name
    extern const std::string CLIENT_USER_NAME_KW;

    /// @brief kw for storing client user  zone
    extern const std::string CLIENT_USER_ZONE_KW;

    /// @brief kw for storing client user priv
    extern const std::string CLIENT_USER_PRIV_KW;

    /// @brief kw for storing proxy user name
    extern const std::string PROXY_USER_NAME_KW;

    /// @brief kw for storing proxy user  zone
    extern const std::string PROXY_USER_ZONE_KW;

    /// @brief kw for storing proxy user priv
    extern const std::string PROXY_USER_PRIV_KW;

    extern const std::string SERVER_CONFIG_FILE;

    class server_properties
    {
    public:
        /// @brief The singleton
        static server_properties& instance();

        /// @brief Read server configuration and fill server_properties::properties
        void capture();

        /// @brief capture server_config.json
        void capture_json(const std::string& _filename);

        template< typename T >
        T& get_property( const std::string& _key ) {
            return config_props_.get< T >( _key );
        }

        template< typename T>
        T& get_property( const configuration_parser::key_path_t& _keys ) {
            return config_props_.get<T>( _keys );
        }

        template< typename T >
        T& set_property( const std::string& _key, const T& _val ) {
            return config_props_.set< T >( _key, _val );
        }

        template< typename T>
        T& set_property( const configuration_parser::key_path_t& _keys, const T& _val ) {
            return config_props_.set<T>( _keys, _val );
        }

        template<typename T>
        T remove( const std::string& _key ) {
            return config_props_.remove<T>( _key );
        }

        void remove( const std::string& _key );

    private:
        server_properties();

        server_properties( server_properties const& ) = delete;
        server_properties& operator=( server_properties const& ) = delete;

        /// @brief properties lookup table
        configuration_parser config_props_;
    }; // class server_properties

    template< typename T >
    T& get_server_property( const std::string& _prop ) {
        return irods::server_properties::instance().get_property<T>(_prop);
    }

    template< typename T >
    T& set_server_property( const std::string& _prop, const T& _val ) {
        return irods::server_properties::instance().set_property<T>(_prop, _val);
    }

    template< typename T >
    T& get_server_property( const configuration_parser::key_path_t& _keys ) {
        return irods::server_properties::instance().get_property<T>(_keys);
    }

    template< typename T >
    T& set_server_property( const configuration_parser::key_path_t& _prop, const T& _val ) {
        return server_properties::instance().set_property<T>(_prop, _val);
    }

    template< typename T >
    T delete_server_property( const std::string& _prop ) {
        return server_properties::instance().remove(_prop);
    }

    void delete_server_property( const std::string& _prop );

    template< typename T >
    T& get_advanced_setting( const std::string& _prop ) {
        return irods::get_server_property<T>(configuration_parser::key_path_t{CFG_ADVANCED_SETTINGS_KW, _prop});
    } // get_advanced_setting

    /// Returns the amount of shared memory that should be allocated for the DNS cache.
    ///
    /// \return An integer representing the size in bytes.
    /// \retval 5000000          If an error occurred or the size was less than or equal to zero.
    /// \retval Configured-Value Otherwise.
    ///
    /// \since 4.2.9
    auto get_dns_cache_shared_memory_size() noexcept -> int;

    /// Returns the DNS cache eviction age from server_config.json.
    ///
    /// \return An integer representing seconds.
    /// \retval 3600             If an error occurred or the age was less than zero.
    /// \retval Configured-Value Otherwise.
    ///
    /// \since 4.2.9
    auto get_dns_cache_eviction_age() noexcept -> int;

    /// Returns the amount of shared memory that should be allocated for the Hostname cache.
    ///
    /// \return An integer representing the size in bytes.
    /// \retval 2500000          If an error occurred or the size was less than or equal to zero.
    /// \retval Configured-Value Otherwise.
    ///
    /// \since 4.2.9
    auto get_hostname_cache_shared_memory_size() noexcept -> int;

    /// Returns the hostname cache eviction age from server_config.json.
    ///
    /// \return An integer representing seconds.
    /// \retval 3600             If an error occurred or the age was less than zero.
    /// \retval Configured-Value Otherwise.
    ///
    /// \since 4.2.9
    auto get_hostname_cache_eviction_age() noexcept -> int;

    /// Parses hosts_config.json into a JSON object if available and stores it in the server
    /// property map with key \p irods::HOSTS_CONFIG_JSON_OBJECT_KW.
    ///
    /// The type of the mapped JSON object is nlohmann::json.
    ///
    /// \since 4.2.9
    auto parse_and_store_hosts_configuration_file_as_json() noexcept -> void;
} // namespace irods

#endif // IRODS_SERVER_PROPERTIES_HPP_

