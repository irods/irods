#ifndef IRODS_SERVER_PROPERTIES_HPP
#define IRODS_SERVER_PROPERTIES_HPP

/// \file

#include "irods_configuration_keywords.hpp"
#include "irods_configuration_parser.hpp"
#include "irods_error.hpp"
#include "irods_exception.hpp"

#include <string>
#include <string_view>

#include "json.hpp"
#include "fmt/format.h"

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

        auto contains(const std::string_view _key) -> bool
        {
            return config_props_.find(_key) != config_props_.end();
        }

        template<typename T>
        T get_property(const std::string& _key)
        {
            auto prop = config_props_.find(_key);
            if (prop != config_props_.end()) {
                return !prop->empty() ? prop->get<T>() : T{};
            }

            THROW(KEY_NOT_FOUND, fmt::format("server properties does not contain key {}", _key));
        }

        template<typename T>
        T get_property(const configuration_parser::key_path_t& _keys)
        {
            auto* tmp = &config_props_;

            for (auto&& k : _keys) {
                if (!tmp->contains(k)) {
                    THROW(KEY_NOT_FOUND, "get_property :: path does not exist");
                }

                tmp = &tmp->at(k);
            }

            return !tmp->empty() ? tmp->get<T>() : T{};
        }

        template<typename T>
        T set_property(const std::string& _key, const T& _val)
        {
            nlohmann::json tmp;
            if (config_props_.contains(_key)) {
                tmp = config_props_.at(_key);
            }

            config_props_[_key] = _val;

            return !tmp.empty() ? tmp.get<T>() : T{};
        }

        template<typename T>
        T set_property(const configuration_parser::key_path_t& _keys, const T& _val)
        {
            auto* tmp = &config_props_;

            for (auto&& k : _keys) {
                if (!tmp->contains(k)) {
                    (*tmp)[k] = nlohmann::json{};
                    tmp = &(*tmp)[k];
                }
                else {
                    tmp = &tmp->at(k);
                }
            }

            auto pre = *tmp;

            *tmp = _val;

            return !pre.empty() ? pre.get<T>() : T{};
        }

        template<typename T>
        T remove( const std::string& _key )
        {
            nlohmann::json tmp;

            if (config_props_.contains(_key)) {
                tmp = config_props_.at(_key);
                config_props_.erase(_key);
            }

            return !tmp.empty() ? tmp.get<T>() : T{};
        }

        void remove( const std::string& _key );

        nlohmann::json& map() noexcept
        {
            return config_props_;
        }

    private:
        server_properties();

        server_properties(const server_properties&) = delete;
        server_properties& operator=(const server_properties&) = delete;

        nlohmann::json config_props_;
    }; // class server_properties

    template<>
    const nlohmann::json& server_properties::get_property<const nlohmann::json&>(const std::string& _key);

    template<>
    const nlohmann::json& server_properties::get_property<const nlohmann::json&>(const configuration_parser::key_path_t& _keys);

    inline bool server_property_exists(const std::string_view _prop)
    {
        return irods::server_properties::instance().contains(_prop);
    }

    template< typename T >
    T get_server_property( const std::string& _prop )
    {
        return irods::server_properties::instance().get_property<T>(_prop);
    }

    template< typename T >
    T set_server_property( const std::string& _prop, const T& _val )
    {
        return irods::server_properties::instance().set_property<T>(_prop, _val);
    }

    template< typename T >
    T get_server_property( const configuration_parser::key_path_t& _keys )
    {
        return irods::server_properties::instance().get_property<T>(_keys);
    }

    template< typename T >
    T set_server_property( const configuration_parser::key_path_t& _prop, const T& _val )
    {
        return server_properties::instance().set_property<T>(_prop, _val);
    }

    template< typename T >
    T delete_server_property( const std::string& _prop )
    {
        return server_properties::instance().remove(_prop);
    }

    void delete_server_property( const std::string& _prop );

    template< typename T >
    T get_advanced_setting( const std::string& _prop )
    {
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

} // namespace irods

#endif // IRODS_SERVER_PROPERTIES_HPP

