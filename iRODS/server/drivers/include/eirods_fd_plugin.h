
#ifndef __EIRODS_FD_PLUGIN_H__
#define __EIRODS_FD_PLUGIN_H__

// =-=-=-=-=-=-=-
// STL Includes
#include <string>

// =-=-=-=-=-=-=-
// dlopen, etc
#include <dlfcn.h>

// =-=-=-=-=-=-=-
// My Includes
#include "eirods_lookup_table.h"

// =-=-=-=-=-=-=-
// iRODS Includes
#include "fileDriver.h"

namespace eirods {

	// =-=-=-=-=-=-=-
	// create a lookup table for the filedriver value type
    class fd_table;
		
	// =-=-=-=-=-=-=-
	// given the name of a file driver, try to load the shared object
	// and then register that fd with the table
	bool load_filedriver_plugin( fd_table& _table, const std::string _fd ); 

}; // namepsace eirods

#endif // __EIRODS_FD_PLUGIN_H__



