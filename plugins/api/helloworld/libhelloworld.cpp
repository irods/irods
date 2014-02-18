/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

// =-=-=-=-=-=-=-
// irods includes
#include "apiHandler.hpp"
#include "irods_stacktrace.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <sstream>
#include <string>
#include <iostream>

extern "C" {

    // =-=-=-=-=-=-=-
    // api function to be referenced by the entry
    int rs_hello_world( rsComm_t* _comm ) {
        rodsLog( LOG_NOTICE, "Dynamic API - HELLO WORLD" );
        return 0;
    }

    // =-=-=-=-=-=-=-
    // factory function to provide instance of the plugin
    irods::api_entry* plugin_factory(
        const std::string& _inst_name,
        const std::string& _context ) {
        // =-=-=-=-=-=-=-
        // create a api def object
        irods::apidef_t def = { 1300,
                                RODS_API_VERSION,
                                NO_USER_AUTH,
                                NO_USER_AUTH,
                                NULL, 0,
                                NULL, 0,
                                0
                              }; // null fcn ptr, handled in delay_load
        // =-=-=-=-=-=-=-
        // create an api object
        irods::api_entry* api = new irods::api_entry( def );
        api->fcn_name_ = "rs_hello_world";
        return api;

    } // plugin_factory

}; // extern "C"

































