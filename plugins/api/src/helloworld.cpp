// =-=-=-=-=-=-=-
// irods includes
#include "apiHandler.hpp"
#include "irods_stacktrace.hpp"
#include "irods_server_api_call.hpp"
#include "irods_re_serialization.hpp"
#include "boost/lexical_cast.hpp"

#include "objStat.h"

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

int call_helloInp_helloOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    helloInp_t*       _inp,
    helloOut_t**      _out ) {
    return _api->call_handler<
               helloInp_t*,
               helloOut_t** >(
                   _comm,
                   _inp,
                   _out );
}

#ifdef RODS_SERVER
static irods::error serialize_helloInp_ptr(
        boost::any               _p,
        irods::re_serialization::serialized_parameter_t& _out) {
    try {
        helloInp_t* tmp = boost::any_cast<helloInp_t*>(_p);
        if(tmp) {
            _out["this"] = boost::lexical_cast<std::string>(tmp->_this);
            _out["that"] = boost::lexical_cast<std::string>(tmp->_that);
        }
        else {
            _out["null_value"] = "null_value";
        }
    }
    catch ( std::exception& ) {
        return ERROR(
                INVALID_ANY_CAST,
                "failed to cast helloInp ptr" );
    }

    return SUCCESS();
} // serialize_helloInp_ptr

static irods::error serialize_helloOut_ptr_ptr(
        boost::any               _p,
        irods::re_serialization::serialized_parameter_t& _out) {
    try {
        helloOut_t** tmp = boost::any_cast<helloOut_t**>(_p);
        if(tmp && *tmp ) {
            helloOut_t*  l = *tmp;
            _out["this"] = boost::lexical_cast<std::string>(l->_this);
            _out["that"] = boost::lexical_cast<std::string>(l->_that);
            _out["value"] = boost::lexical_cast<std::string>(l->_other._value);
        }
        else {
            _out["null_value"] = "null_value";
        }
    }
    catch ( std::exception& ) {
        return ERROR(
                INVALID_ANY_CAST,
                "failed to cast helloOut ptr ptr" );
    }

    return SUCCESS();
} // serialize_helloOut_ptr_ptr
#endif


#ifdef RODS_SERVER
    #define CALL_HELLOINP_HELLO_OUT call_helloInp_helloOut
#else
    #define CALL_HELLOINP_HELLO_OUT NULL
#endif

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

extern "C" {
    // =-=-=-=-=-=-=-
    // factory function to provide instance of the plugin
    irods::api_entry* plugin_factory(
        const std::string&,     //_inst_name
        const std::string& ) { // _context
        // =-=-=-=-=-=-=-
        // create a api def object
        irods::apidef_t def = { 1300,             // api number
                                RODS_API_VERSION, // api version
                                NO_USER_AUTH,     // client auth
                                NO_USER_AUTH,     // proxy auth
                                "HelloInp_PI", 0, // in PI / bs flag
                                "HelloOut_PI", 0, // out PI / bs flag
                                std::function<
                                    int( rsComm_t*,helloInp_t*,helloOut_t**)>(
                                        rs_hello_world), // operation
								"api_hello_world",    // operation name
                                0,  // null clear fcn
                                (funcPtr)CALL_HELLOINP_HELLO_OUT
                              };
        // =-=-=-=-=-=-=-
        // create an api object
        irods::api_entry* api = new irods::api_entry( def );

#ifdef RODS_SERVER
        irods::re_serialization::add_operation(
                typeid(helloInp_t*),
                serialize_helloInp_ptr );

        irods::re_serialization::add_operation(
                typeid(helloOut_t**),
                serialize_helloOut_ptr_ptr );
#endif

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
