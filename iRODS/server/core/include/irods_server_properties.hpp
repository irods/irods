/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

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
            return PASSMSG( "server_properties::get_property", ret );
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
