


#ifndef __EIRODS_RESOURCE_TYPES_H__

// =-=-=-=-=-=-=-
// Boost Includes
#include <boost/shared_ptr.hpp>
#include <boost/any.hpp>

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_plugin_base.h"
#include "eirods_lookup_table.h"

namespace eirods {

	// =-=-=-=-=-=-=-
    // resource operation function signature, helpful typedefs
	class resource;
	typedef boost::shared_ptr< resource >                          resource_ptr;
	typedef lookup_table<boost::any>                               resource_property_map;
	typedef	lookup_table< std::pair< std::string, resource_ptr > > resource_child_map;

	typedef error ((*resource_operation)( resource_property_map*, resource_child_map*, ... ));
	typedef error (*resource_maintenance_operation)();

}; // namespace

#define __EIRODS_RESOURCE_TYPES_H__
#endif // __EIRODS_RESOURCE_TYPES_H__



