// =-=-=-=-=-=-=-
// E-iRODS Includes
#include "msParam.hpp"
#include "reGlobalsExtern.hpp"
#include "irods_ms_plugin.hpp"

// =-=-=-=-=-=-=-
// STL Includes
#include <iostream>

extern "C" {

    // =-=-=-=-=-=-=-
    // 1. Set the version of the plugin.  1.0 is currently the standard
    double PLUGIN_INTERFACE_VERSION=1.0;
	
    // =-=-=-=-=-=-=-
    // 2. Write a standard issue microservice	
    int irods_msvc_test( msParam_t* _a, msParam_t* _b, msParam_t* _c, ruleExecInfo_t* _rei ) {
        std::cout << "irods_msvc_test :: " << parseMspForStr( _a ) << " " << parseMspForStr( _b ) << " " << parseMspForStr( _c ) << std::endl;
        return 0;
    }

    // =-=-=-=-=-=-=-
    // 3.  Create the plugin factory function which will return a microservice
    //     table entry 
    irods::ms_table_entry*  plugin_factory( ) {
        // =-=-=-=-=-=-=-
        // 4.  allocate a microservice plugin which takes the number of function 
        //     params as a parameter to the constructor 
        irods::ms_table_entry* msvc = new irods::ms_table_entry( 3 );	

        // =-=-=-=-=-=-=-
        // 5. add the microservice function as an operation to the plugin
        //    the first param is the name / key of the operation, the second
        //    is the name of the function which will be the microservice
        msvc->add_operation( "irods_msvc_test", "irods_msvc_test" );
        
        // =-=-=-=-=-=-=-
        // 6. return the newly created microservice plugin
        return msvc;
    }

}; // extern "C" 



