// =-=-=-=-=-=-=-
#include "msParam.hpp"
#include "reGlobalsExtern.hpp"
#include "irods_ms_plugin.hpp"

// =-=-=-=-=-=-=-
// STL Includes
#include <iostream>

extern "C" {

    // =-=-=-=-=-=-=-
    // 1. Write a standard issue microservice
    int irods_msvc_test( msParam_t* _a, msParam_t* _b, msParam_t* _c, msParam_t* _out, ruleExecInfo_t* _rei ) {
        std::cout << "irods_msvc_test :: " << parseMspForStr( _a ) << " " << parseMspForStr( _b ) << " " << parseMspForStr( _c ) << std::endl;
        std::string astr = parseMspForStr( _a );
        std::string bstr = parseMspForStr( _b );
        std::string cstr = parseMspForStr( _c );
        std::string my_str = "irods_msvc_test :: " + astr + " " + bstr + " " + cstr + "\n(The preceding line is also written into the server log.)";
        fillStrInMsParam( _out, my_str.c_str() );
        return 0;
    }

    // =-=-=-=-=-=-=-
    // 2.  Create the plugin factory function which will return a microservice
    //     table entry
    irods::ms_table_entry*  plugin_factory( ) {
        // =-=-=-=-=-=-=-
        // 3.  allocate a microservice plugin which takes the number of function
        //     params as a parameter to the constructor
        irods::ms_table_entry* msvc = new irods::ms_table_entry( 4 );

        // =-=-=-=-=-=-=-
        // 4. add the microservice function as an operation to the plugin
        //    the first param is the name / key of the operation, the second
        //    is the name of the function which will be the microservice
        msvc->add_operation( "irods_msvc_test", "irods_msvc_test" );

        // =-=-=-=-=-=-=-
        // 5. return the newly created microservice plugin
        return msvc;
    }

}; // extern "C"



