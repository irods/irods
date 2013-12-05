// =-=-=-=-=-=-=-
// E-iRODS Includes
#include "msParam.h"
#include "reGlobalsExtern.h"
#include "irods_ms_plugin.h"

// =-=-=-=-=-=-=-
// STL Includes
#include <iostream>

extern "C" {

    // =-=-=-=-=-=-=-
    // 1. Write a standard issue microservice
    int irods_msvc_test( msParam_t* _a, msParam_t* _b, msParam_t* _c, ruleExecInfo_t* _rei ) {
        std::cout << "irods_msvc_test :: " << parseMspForStr( _a ) << " " << parseMspForStr( _b ) << " " << parseMspForStr( _c ) << std::endl;
        return 0;
    }

    // =-=-=-=-=-=-=-
    // 2.  Create the plugin factory function which will return a microservice
    //     table entry containing the microservice function pointer, the number
    //     of parameters that the microservice takes and the name of the micro
    //     service.  this will be called by the plugin loader in the irods server
    //     to create the entry to the table when the plugin is requested.
    irods::ms_table_entry*  plugin_factory( ) {
        irods::ms_table_entry* msvc = new irods::ms_table_entry( 3 );
        msvc->add_operation( "irods_msvc_test", "irods_msvc_test" );
        return msvc;
    }

}; // extern "C"



