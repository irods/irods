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

#include <boost/format.hpp>
#include <boost/any.hpp>
#include <map>

namespace irods {

/// @brief kw for server property map storing strict acl configuration
    static const std::string STRICT_ACL_KW( "strict_acls" );

/// @brief kw for server property map stating this is an agent-agent conn
    static const std::string AGENT_CONN_KW( "agent_conn" );

/// @brief kw for server property map for encryption key
    static const std::string AGENT_KEY_KW( "agent_key" );

/// @brief kw for storing the process id of the rule engine server
    static const std::string RE_PID_KW( "rule_engine_process_id" );

/// @brief kw for storing client user name
    static const std::string CLIENT_USER_NAME_KW( "client_user_name" );

/// @brief kw for storing client user  zone
    static const std::string CLIENT_USER_ZONE_KW( "client_user_zone" );

/// @brief kw for storing client user priv
    static const std::string CLIENT_USER_PRIV_KW( "client_user_priv" );

/// @brief kw for storing proxy user name
    static const std::string PROXY_USER_NAME_KW( "proxy_user_name" );

/// @brief kw for storing proxy user  zone
    static const std::string PROXY_USER_ZONE_KW( "proxy_user_zone" );

/// @brief kw for storing proxy user priv
    static const std::string PROXY_USER_PRIV_KW( "proxy_user_priv" );

/// @brief kw for storing enhanced logging setting 
    static const std::string ENHANCED_LOGGING_KW( "enhanced_logging" );

    class server_properties {

        public:
            /**
             * @brief Access method for the singleton
             */
            static server_properties& getInstance();

            /**
             * @brief Read server configuration and fill server_properties::properties
             */
            void capture( );

            /**
             * @brief capture the legacy version: server.config
             */
            void capture_legacy();

            /**
             * @brief capture the new json version: server_config.json
             */
            void capture_json( const std::string& );

            /**
             * @brief Get a property from the map if it exists.  catch the exception in the case where
             * the template types may not match and return success/fail
             */
            template< typename T >
            error get_property( const std::string& _key, T& _val ) {
                try {
                    _val = config_props_.get< T >( _key );
                } catch ( const irods::exception& e ) {
                    std::map<std::string, std::string>::iterator find_it = key_map_.find( _key );
                    if ( find_it != key_map_.end() ) {
                        try {
                            _val = config_props_.get< T >( find_it->second );
                        } catch ( const irods::exception& e ) {
                            return irods::error(e);
                        }
                    }
                    else {
                        return irods::error(e);
                    }
                }
                return SUCCESS();
            }

            template< typename T >
            error set_property( const std::string& _key, const T& _val ) {
                try {
                    config_props_.set< T >( _key, _val );
                } catch ( const irods::exception& e ) {
                    return irods::error(e);
                }
                return SUCCESS();
            }

            error delete_property( const std::string& _key );

        private:
            // Disable constructors
            server_properties( server_properties const& );
            server_properties( );
            void operator=( server_properties const& );

            /**
             * @brief properties lookup table
             */
            configuration_parser config_props_;

            /// @brief map of old keys to new keys
            std::map< std::string, std::string > key_map_;

    }; // class server_properties

    template< typename T >
    error get_server_property( const std::string& _prop, T& _val ) {
        try {
            irods::error ret = irods::server_properties::getInstance().get_property< T > (
                    _prop,
                    _val );
            if ( !ret.ok() ) {
                return PASS( ret );
            }
        } catch ( const irods::exception& e ) {
            return irods::error(e);
        }

        return SUCCESS();

    } // get_server_property

    template< typename T >
    error set_server_property( const std::string& _prop, const T& _val ) {
        try {
            irods::error ret = irods::server_properties::getInstance().set_property< T > (
                    _prop,
                    _val );
            if ( !ret.ok() ) {
                return PASS( ret );
            }
        } catch ( const irods::exception& e ) {
            return irods::error(e);
        }

        return SUCCESS();

    } // set_server_property

    error delete_server_property( const std::string& _prop );

    template< typename T >
    error get_advanced_setting(
        const std::string& _prop,
        T&                 _val ) {
        std::map<std::string, boost::any> adv_set;
        irods::error ret = irods::get_server_property< std::map<std::string, boost::any> > (
                  CFG_ADVANCED_SETTINGS_KW,
                  adv_set );
        if ( !ret.ok() ) {
            return PASS( ret );

        }

        std::map<std::string, boost::any>::iterator find_it = adv_set.find( _prop );
        if ( find_it == adv_set.end() ) {
            std::string msg( "missing [" );
            msg += _prop;
            msg += "]";
            return ERROR(
                       KEY_NOT_FOUND,
                       msg );

        }

        try {
            _val = boost::any_cast<T>( find_it->second );
        } catch ( const boost::bad_any_cast& ) {
            return ERROR(INVALID_ANY_CAST, "bad any cast in get_advanced_setting");
        }
        return SUCCESS();

    } // get_advanced_setting

} // namespace irods

#endif /* IRODS_SERVER_PROPERTIES_HPP_ */
