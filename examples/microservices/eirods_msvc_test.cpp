// =-=-=-=-=-=-=-
// E-iRODS Includes
#include "msParam.h"
#include "reGlobalsExtern.h"
#include "eirods_ms_plugin.h"

// =-=-=-=-=-=-=-
// STL Includes
#include <iostream>

extern "C" {

    double EIRODS_PLUGIN_INTERFACE_VERSION=1.0;
    
    // =-=-=-=-=-=-=-
    // 1.  Create your microservice
    int eirods_msvc_test( msParam_t* _a, msParam_t* _b, msParam_t* _c, ruleExecInfo_t* _rei ) {
        std::cout << "eirods_msvc_test :: " << parseMspForStr( _a ) << " " << parseMspForStr( _b ) << " " << parseMspForStr( _c ) << std::endl;
        return 0;
    }

    // =-=-=-=-=-=-=-
    // 2.  Create the plugin factory function which will return a microservice
    //     table entry containing the name of the microservice and the number
    //     of parameters that the microservice takes.  This will be called by
    //     the plugin loader in the iRODS server to create the entry in the
    //     table when the plugin is requested.
    eirods::ms_table_entry*  plugin_factory( ) {
        return new eirods::ms_table_entry( "eirods_msvc_test", 3 );
    }

}; // extern "C"

