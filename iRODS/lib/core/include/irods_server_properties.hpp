/*
 * irods_server_properties.hpp
 *
 *  Created on: Jan 15, 2014
 *      Author: adt
 */

#ifndef IRODS_SERVER_PROPERTIES_HPP_
#define IRODS_SERVER_PROPERTIES_HPP_


#include "irods_lookup_table.hpp"
#include "irods_configuration_parser.hpp"
#include "irods_configuration_keywords.hpp"
#include "irods_exception.hpp"

namespace irods {

/// @brief kw for server property map storing strict acl configuration
    const std::string STRICT_ACL_KW( "strict_acls" );

/// @brief kw for server property map stating this is an agent-agent conn
    const std::string AGENT_CONN_KW( "agent_conn" );

/// @brief kw for server property map for encryption key
    const std::string AGENT_KEY_KW( "agent_key" );

/// @brief kw for storing the process id of the rule engine server
    const std::string RE_PID_KW( "rule_engine_process_id" );

/// @brief kw for storing the process id of the xmessage server
    const std::string XMSG_PID_KW( "x_message_process_id" );

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
             * @brief capture the legacy version: server.config
             */
            void capture_legacy();


            /**
             * @brief Read server configuration if it has not been read already.
             */
            void capture_if_needed();

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
                    capture_if_needed();
                } catch ( const exception& e ) {
                    return ERROR( e.code(), e.what() );
                }

                error ret = config_props_.get< T >( _key, _val );
                if ( !ret.ok() ) {
                    if ( key_map_.has_entry( _key ) ) {
                        ret = config_props_.get< T >( key_map_[ _key ], _val );
                    }
                }
                return PASS( ret );
            }

            template< typename T >
            error set_property( const std::string& _key, const T& _val ) {
                try {
                    capture_if_needed();
                } catch ( const exception& e ) {
                    return ERROR( e.code(), e.what() );
                }

                error ret = config_props_.set< T >( _key, _val );
                if ( !ret.ok() ) {
                    ret = config_props_.set< T >( key_map_[ _key ], _val );
                }
                return PASS( ret );
            }

            error delete_property( const std::string& _key ) {
                try {
                    capture_if_needed();
                } catch ( const exception& e ) {
                    return ERROR( e.code(), e.what() );
                }

                size_t n = config_props_.erase( _key );
                if ( n != 1 ) {
                    std::string msg( "failed to erase key: " );
                    msg += _key;
                    return ERROR( UNMATCHED_KEY_OR_INDEX, _key );
                }
                else {
                    return SUCCESS();
                }
            }

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
            lookup_table< std::string > key_map_;
            bool captured_;

    }; // class server_properties

    template< typename T >
    error get_server_property(
        const std::string& _prop,
        T&                 _val ) {
        error ret = server_properties::instance().get_property< T > (
                  _prop,
                  _val );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        return SUCCESS();

    } // get_server_property

    template< typename T >
    error set_server_property(
        const std::string& _prop,
        const T&           _val ) {
        error ret = server_properties::instance().set_property< T > (
                  _prop,
                  _val );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        return SUCCESS();

    } // set_server_property

    static error delete_server_property(
        const std::string& _prop ) {
        error ret = server_properties::instance().delete_property(
                  _prop );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        return SUCCESS();

    } // delete_server_property

    template< typename T >
    error get_advanced_setting(
        const std::string& _prop,
        T&                 _val ) {

        lookup_table<boost::any> adv_set;
        error ret = get_server_property< lookup_table<boost::any> > (
                  CFG_ADVANCED_SETTINGS_KW,
                  adv_set );
        if ( !ret.ok() ) {
            return PASS( ret );

        }

        ret = adv_set.get<T>(
                   _prop,
                   _val );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        return SUCCESS();

    } // get_advanced_setting

} // namespace irods

#endif /* IRODS_SERVER_PROPERTIES_HPP_ */
