/*
 * irods_environment_properties.hpp
 *
 */

#ifndef IRODS_SERVER_PROPERTIES_HPP_
#define IRODS_SERVER_PROPERTIES_HPP_


#include "irods_lookup_table.hpp"
#include "irods_configuration_parser.hpp"
#include "irods_configuration_keywords.hpp"

namespace irods {

    const std::string STRICT_ACL_KW( "strict_acls" );

/// @brief kw for server property map stating this is an agent-agent conn
    const std::string AGENT_CONN_KW( "agent_conn" );

/// @brief kw for server property map for encryption key
    const std::string AGENT_KEY_KW( "agent_key" );

    class environment_properties {

    public:
        /**
         * @brief Access method for the singleton
         */
        static environment_properties& getInstance();

        /**
         * @brief Read server configuration and fill environment_properties::properties
         */
        error capture( );

        /**
         * @brief Read server configuration if it has not been read already.
         **/
        error capture_if_needed();

        /**
         * @brief Get a property from the map if it exists.  catch the exception in the case where
         * the template types may not match and return success/fail
         */
        template< typename T >
        error get_property( const std::string& _key, T& _val ) {
            error ret = config_props_.get< T >( _key, _val );
            if( !ret.ok() ) {
                ret = config_props_.get< T >( key_map_[ _key ], _val );
            }
            return PASS( ret );
        }

        template< typename T >
        error set_property( const std::string& _key, const T& _val ) {
            error ret = config_props_.set< T >( _key, _val );
            if( !ret.ok() ) {
                ret = config_props_.set< T >( key_map_[ _key ], _val );
            }
            return PASS( ret );
        }

        error delete_property( const std::string& _key ) {
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
        environment_properties( environment_properties const& );
        environment_properties( );
        void operator=( environment_properties const& );

        /**
         * @brief capture the legacy version: server.config
         */
        error capture_legacy( const std::string& );

        /**
         * @brief capture the new json version: server_config.json
         */
        error capture_json( const std::string& );


        /**
         * @brief properties lookup table
         */
        configuration_parser config_props_;

        /// @brief map of old keys to new keys
        lookup_table< std::string > key_map_;
        bool captured_;

        const std::string LEGACY_ENV_FILE;
        const std::string JSON_ENV_FILE;


    }; // class environment_properties


} // namespace irods

#endif /* IRODS_SERVER_PROPERTIES_HPP_ */
