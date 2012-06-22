// =-=-=-=-=-=-=-
// E-iRODS Includes
#include "msParam.h"
#include "reGlobalsExtern.h"
#include "eirods_ms_plugin.h"

// =-=-=-=-=-=-=-
// STL Includes
#include <iostream>

extern "C" {

    int EIRODS_PLUGIN_VERSION=1.0;
	
    // =-=-=-=-=-=-=-
	// 1. Write a standard issue microservice	
	int eirods_msvc_test( msParam_t* _a, msParam_t* _b, msParam_t* _c, ruleExecInfo_t* _rei ) {
		std::cout << "eirods_msvc_test :: " << parseMspForStr( _a ) << " " << parseMspForStr( _b ) << " " << parseMspForStr( _c ) << std::endl;
		return 0;
	}

    // =-=-=-=-=-=-=-
	// 2.  Create the plugin factory function which will return a microservice
	//     table entry containing the microservice function pointer, the number
	//     of parameters that the microservice takes and the name of the micro
	//     service.  this will be called by the plugin loader in the irods server
	//     to create the entry to the table when the plugin is requested.
	eirods::ms_table_entry*  plugin_factory( ) {
	    return new eirods::ms_table_entry( "eirods_msvc_test", 3 );	
	}

}; // extern "C" 



