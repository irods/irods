/*
 * eirods_catalog_properties.h
 *
 *  Created on: Oct 9, 2013
 *      Author: adt
 */

#ifndef CATALOG_PROPERTIES_H_
#define CATALOG_PROPERTIES_H_


// =-=-=-=-=-=-=-
// eirods includes

#include "eirods_lookup_table.h"


namespace eirods {

	// =-=-=-=-=-=-=-
	/// @brief This setting controls how backslashes in strings literals are treated.
	// See http://www.postgresql.org/docs/9.3/static/runtime-config-compatible.html#GUC-STANDARD-CONFORMING-STRINGS
	const std::string STANDARD_CONFORMING_STRINGS("standard_conforming_strings");


	class catalog_properties {

	public:
		// =-=-=-=-=-=-=-
		// Access singleton through its getInstance() method
		static catalog_properties& getInstance();

		// =-=-=-=-=-=-=-
		// Query for iCAT settings and fill catalog_properties::properties
		error capture();

	    // =-=-=-=-=-=-=-
	    /// @brief get a property from the map if it exists.  catch the exception in the case where
	    // the template types may not match and return success/fail
	    template< typename T >
	    error get_property( const std::string& _key, T& _val ) {
	        error ret = properties.get< T >( _key, _val );
	        return PASSMSG( "catalog_properties::get_property", ret );
	    } // get_property


	private:
	    // =-=-=-=-=-=-=-
	    // Disable constructors
		catalog_properties() {};
		catalog_properties(catalog_properties const&);
		void operator=(catalog_properties const&);

		// =-=-=-=-=-=-=-
		// properties
		lookup_table<boost::any> properties;

	}; // class catalog_properties


} // namespace eirods


#endif /* CATALOG_PROPERTIES_H_ */



