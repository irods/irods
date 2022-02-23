/*
 * irods_catalog_properties.h
 *
 *  Created on: Oct 9, 2013
 *      Author: adt
 */

#ifndef CATALOG_PROPERTIES_HPP_
#define CATALOG_PROPERTIES_HPP_


// =-=-=-=-=-=-=-

#include "irods/icatStructs.hpp"
#include "irods/irods_lookup_table.hpp"
#include "irods/rodsErrorTable.h"
#include <unordered_map>
#include <sstream>


namespace irods {

// =-=-=-=-=-=-=-
/// @brief This setting controls how backslashes in strings literals are treated.
// See http://www.postgresql.org/docs/9.3/static/runtime-config-compatible.html#GUC-STANDARD-CONFORMING-STRINGS
    const std::string STANDARD_CONFORMING_STRINGS( "standard_conforming_strings" );


    class catalog_properties {
        public:

            /**
             * @brief The singleton
             */
            static catalog_properties& instance();

            /*
             * @brief Query for iCAT settings and fill catalog_properties::instance
             */
            void capture( icatSessionStruct* );

            /*
             * @brief Query for iCAT settings if it has not already been queried
             */
            void capture_if_needed( icatSessionStruct* );

            /**
             * @brief get a property from the map if it exists.  catch the exception in the case where
             * the template types may not match and return success/fail
             */
            template< typename T >
            error get_property( const std::string& _key, T& _val ) {
                if ( !captured_ ) {
                    return ERROR( SYS_INTERNAL_ERR, "'capture' must be called on the catalog_properties instance before properties may be accessed." );
                }
                std::unordered_map<std::string, boost::any>::iterator it = properties.find( _key );
                if ( it == properties.end() ) {
                    std::stringstream msg;
                    msg << "Catalog properties map does not contain the key \"" << _key << "\".";
                    return ERROR( SYS_INTERNAL_ERR, msg.str() );
                }
                try {
                    _val = boost::any_cast<T>( it->second );
                } catch( const boost::bad_any_cast& e ) {
                    std::stringstream msg;
                    msg << "The value in the catalog properties map at \"" << _key << "\" is not of the appropriate type.";
                    return ERROR( SYS_INTERNAL_ERR, msg.str() );
                }

                return SUCCESS();
            } // get_property


            std::unordered_map<std::string, boost::any> properties;
        private:
            // =-=-=-=-=-=-=-
            // Disable constructors
            catalog_properties() : captured_( false ) {};
            catalog_properties( catalog_properties const& );
            void operator=( catalog_properties const& );

            // =-=-=-=-=-=-=-
            // properties
            bool captured_;

    }; // class catalog_properties


    template< typename T >
    error get_catalog_property(
        const std::string& _key,
        T&                 _val ) {
        irods::error ret = irods::catalog_properties::instance().get_property<T>(
                _key,
                _val );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        return SUCCESS();

    } // get_catalog_property


} // namespace irods


#endif /* CATALOG_PROPERTIES_HPP_ */
