/*
 * irods_server_properties.hpp
 *
 *  Created on: Jan 15, 2014
 *      Author: adt
 */

#ifndef IRODS_SERVER_PROPERTIES_HPP_
#define IRODS_SERVER_PROPERTIES_HPP_


#include "irods_exception.hpp"
#include "irods_error.hpp"
#include "irods_configuration_parser.hpp"
#include "irods_configuration_keywords.hpp"
#include "irods_exception.hpp"

#include <boost/format.hpp>
#include <boost/any.hpp>
#include <map>

namespace irods {

/// @brief kw for server property map storing strict acl configuration
    const std::string STRICT_ACL_KW( "strict_acls" );

/// @brief kw for server property map stating this is an agent-agent conn
    const std::string AGENT_CONN_KW( "agent_conn" );

/// @brief kw for server property map for encryption key
    const std::string AGENT_KEY_KW( "agent_key" );

/// @brief kw for storing the process id of the rule engine server
    const std::string RE_PID_KW( "rule_engine_process_id" );

/// @brief kw for storing client user name
    const std::string CLIENT_USER_NAME_KW( "client_user_name" );

/// @brief kw for storing client user  zone
    const std::string CLIENT_USER_ZONE_KW( "client_user_zone" );

/// @brief kw for storing client user priv
    const std::string CLIENT_USER_PRIV_KW( "client_user_priv" );

/// @brief kw for storing proxy user name
    const std::string PROXY_USER_NAME_KW( "proxy_user_name" );

/// @brief kw for storing proxy user  zone
    const std::string PROXY_USER_ZONE_KW( "proxy_user_zone" );

/// @brief kw for storing proxy user priv
    const std::string PROXY_USER_PRIV_KW( "proxy_user_priv" );

    const std::string SERVER_CONFIG_FILE( "server_config.json" );

    class server_properties {
        public:

            /**
            * @brief The singleton
            */
            static server_properties& instance();

            /**
             * @brief Read server configuration and fill server_properties::properties
             */
            void capture();

            /**
             * @brief capture server_config.json
             */
            void capture_json( const std::string& );

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
            // Disable constructors
            server_properties( server_properties const& );
            server_properties( );
            void operator=( server_properties const& );

            /**
             * @brief properties lookup table
             */
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

} // namespace irods

#endif /* IRODS_SERVER_PROPERTIES_HPP_ */
