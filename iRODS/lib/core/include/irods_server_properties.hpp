/*
 * irods_server_properties.hpp
 *
 *  Created on: Jan 15, 2014
 *      Author: adt
 */

#ifndef IRODS_SERVER_PROPERTIES_HPP_
#define IRODS_SERVER_PROPERTIES_HPP_


#include "irods_lookup_table.hpp"


namespace irods {

/// @brief kw for server property map stating this is an agent-agent conn
    const std::string AGENT_CONN_KW( "agent_conn" );

/// @brief kw for server property map for encryption key
    const std::string AGENT_KEY_KW( "agent_key" );

    class server_properties {

    public:
        /**
         * @brief Access method for the singleton
         */
        static server_properties& getInstance();

        /**
         * @brief Read server configuration and fill server_properties::properties
         */
        error capture();

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
            error ret = properties.get< T >( _key, _val );
            return PASS( ret );
        }

        template< typename T >
        error set_property( const std::string& _key, const T& _val ) {
            error ret = properties.set< T >( _key, _val );
            return PASS( ret );
        }

        error delete_property( const std::string& _key ) {
            size_t n = properties.erase( _key );
            if ( n != 1 ) {
                std::string msg( "failed to erase key: " );
                msg += _key;
                return ERROR( UNMATCHED_KEY_OR_INDEX, _key );
            }
            else {
                return SUCCESS();
            }
        }

        typedef lookup_table<boost::any>::iterator iterator;
        iterator begin() {
            return properties.begin();
        }

        iterator end() {
            return properties.end();
        }

    private:
        // Disable constructors
        server_properties(): captured_( false ) {};
        server_properties( server_properties const& );
        void operator=( server_properties const& );

        /**
         * @brief properties lookup table
         */
        lookup_table<boost::any> properties;

        bool captured_;

    }; // class server_properties


} // namespace irods

#endif /* IRODS_SERVER_PROPERTIES_HPP_ */
