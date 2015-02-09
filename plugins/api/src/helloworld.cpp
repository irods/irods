// =-=-=-=-=-=-=-
// irods includes
#include "apiHandler.hpp"
#include "irods_stacktrace.hpp"
#include "irods_server_api_call.hpp"

#include "objStat.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <sstream>
#include <string>
#include <iostream>

typedef struct {
    int  _this;
    char _that [64];
} helloInp_t;

typedef struct {
    double _value;
} otherOut_t;

typedef struct {
    int  _this;
    char _that [64];
    otherOut_t _other;
} helloOut_t;

#define HelloInp_PI "int _this; str _that[64];"
#define OtherOut_PI "double _value;"
#define HelloOut_PI "int _this; str _that[64]; struct OtherOut_PI;"

extern "C" {

    // =-=-=-=-=-=-=-
    // api function to be referenced by the entry
    int rs_hello_world( rsComm_t*, helloInp_t* _inp, helloOut_t** _out ) {
        rodsLog( LOG_NOTICE, "Dynamic API - HELLO WORLD" );

        ( *_out ) = ( helloOut_t* )malloc( sizeof( helloOut_t ) );
        ( *_out )->_this = 42;
        strncpy( ( *_out )->_that, "hello, world.", 63 );
        ( *_out )->_other._value = 128.0;

        rodsLog( LOG_NOTICE, "Dynamic API - this [%d] that [%s]", _inp->_this, _inp->_that );
        rodsLog( LOG_NOTICE, "Dynamic API - DONE" );

        return 0;
    }

    // =-=-=-=-=-=-=-
    // factory function to provide instance of the plugin
    irods::api_entry* plugin_factory(
        const std::string&,     //_inst_name
        const std::string& ) { // _context
        // =-=-=-=-=-=-=-
        // create a api def object
        irods::apidef_t def = { 1300,
                                RODS_API_VERSION,
                                NO_USER_AUTH,
                                NO_USER_AUTH,
                                "HelloInp_PI", 0,
                                "HelloOut_PI", 0,
                                0, // null fcn ptr, handled in delay_load
                                0  // null clear fcn
                              };
        // =-=-=-=-=-=-=-
        // create an api object
        irods::api_entry* api = new irods::api_entry( def );

#ifdef RODS_SERVER
        api->fcn_name_ = "rs_hello_world";
#endif // RODS_SERVER

        // =-=-=-=-=-=-=-
        // assign the pack struct key and value
        api->in_pack_key   = "HelloInp_PI";
        api->in_pack_value = HelloInp_PI;

        api->out_pack_key   = "HelloOut_PI";
        api->out_pack_value = HelloOut_PI;

        api->extra_pack_struct[ "OtherOut_PI" ] = OtherOut_PI;

        return api;

    } // plugin_factory

}; // extern "C"
