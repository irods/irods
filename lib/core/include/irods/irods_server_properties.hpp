#ifndef IRODS_SERVER_PROPERTIES_HPP
#define IRODS_SERVER_PROPERTIES_HPP

/// \file

#include "irods/irods_configuration_keywords.hpp"
#include "irods/irods_configuration_parser.hpp"
#include "irods/irods_error.hpp"
#include "irods/irods_exception.hpp"

#include <mutex>
#include <shared_mutex>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>
#include <fmt/format.h>

namespace irods
{
    /// @brief kw for server property map storing strict acl configuration
    extern const std::string STRICT_ACL_KW;

    /// @brief kw for server property map stating this is an agent-agent conn
    extern const std::string AGENT_CONN_KW;

    /// @brief kw for server property map for encryption key
    extern const std::string AGENT_KEY_KW;

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

        /// \brief Read server configuration, replacing existing keys instead of deleting them.
        /// \returns a json array containing a json patch
        nlohmann::json reload();

        auto contains(const std::string_view _key) -> bool
        {
            auto lock = acquire_read_lock();
            return config_props_.find(_key) != config_props_.end();
        }

        template<typename T>
        T get_property(const std::string& _key)
        {
            auto lock = acquire_read_lock();
            auto prop = config_props_.find(_key);
            if (prop != config_props_.end()) {
                return !prop->empty() ? prop->get<T>() : T{};
            }

            THROW(KEY_NOT_FOUND, fmt::format("key does not exist [{}].", _key));
        }

        template<typename T>
        T get_property(const configuration_parser::key_path_t& _keys)
        {
            auto lock = acquire_read_lock();
            auto* tmp = &config_props_;

            for (auto&& k : _keys) {
                if (!tmp->contains(k)) {
                    THROW(KEY_NOT_FOUND, fmt::format("path does not exist [{}].", fmt::join(_keys, ".")));
                }

                tmp = &tmp->at(k);
            }

            return !tmp->empty() ? tmp->get<T>() : T{};
        }

        template<typename T>
        T set_property(const std::string& _key, const T& _val)
        {
            auto lock = acquire_write_lock();
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
            auto lock = acquire_write_lock();
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
            auto lock = acquire_write_lock();
            nlohmann::json tmp;

            if (config_props_.contains(_key)) {
                tmp = config_props_.at(_key);
                config_props_.erase(_key);
            }

            return !tmp.empty() ? tmp.get<T>() : T{};
        }

        void remove( const std::string& _key );

    private:
        class [[nodiscard("Do not discard lock!")]] readonly_config_handle
        {
          public:
            readonly_config_handle(const nlohmann::json& json, std::shared_mutex& mutex)
                : server_config{json}
                , lock{mutex}
            {
            }

            readonly_config_handle(const readonly_config_handle&) = delete;
            auto operator=(const readonly_config_handle&)->readonly_config_handle& = delete;

            [[nodiscard]] auto get_json() const& noexcept->const nlohmann::json&
            {
                return server_config;
            }

          private:
            const nlohmann::json& server_config;
            std::shared_lock<std::shared_mutex> lock;
        };

    public:
        /// \brief Acquire a read-only handle containing the server configuration.
        ///
        /// This function acquires a read-only handle that ensures, for the lifetime of the handle, the server
        /// configuration cannot be modified by other threads of execution.
        ///
        /// Example of usage is as follows:
        /// \code{.cpp}
        /// const auto config_handle{irods::server_properties::server_properties::instance().map()};
        /// const auto& config{config_handle.get_json()};
        /// \endcode
        ///
        /// \warning Do not store the handle in a `const auto&` variable. This will cause the lifetime of the handle
        ///          to extend past what is normally expected, and result in code containing unexpected behavior.
        ///
        /// \since 4.3.1
        readonly_config_handle map() const noexcept
        {
            return readonly_config_handle{config_props_, property_mutex_};
        }

    private:
        /// \brief Acquire a read lock, that is, a lock that is not exclusive, that is why it returns a shared_lock
        /// here.
        std::shared_lock<std::shared_mutex> acquire_read_lock() const
        {
            return std::shared_lock<std::shared_mutex>(property_mutex_);
        }

        /// \brief Acquire an exclusive lock for writing. While as many threads may read the properties object,
        /// writing to the underlying std::map(or close enough, nlohmann uses red black trees) is not guaranteed to be
        /// thread safe
        ///
        /// The unique lock is necessary here over scoped_lock or move_lock because the unique lock
        /// supports move semantics.
        std::unique_lock<std::shared_mutex> acquire_write_lock() const
        {
            return std::unique_lock(property_mutex_);
        }

        server_properties();

        server_properties(const server_properties&) = delete;
        server_properties& operator=(const server_properties&) = delete;

        nlohmann::json config_props_;
        mutable std::shared_mutex property_mutex_;
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
        return irods::get_server_property<T>(configuration_parser::key_path_t{KW_CFG_ADVANCED_SETTINGS, _prop});
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

