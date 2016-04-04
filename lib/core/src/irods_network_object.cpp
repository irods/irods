// =-=-=-=-=-=-=-
#include "irods_network_object.hpp"
#include "irods_network_manager.hpp"

#include "rodsKeyWdDef.h"
#include "rcMisc.h"

namespace irods {

// =-=-=-=-=-=-=-
// public - ctor
    network_object::network_object() :
        socket_handle_( 0 ) {

    } // ctor

// =-=-=-=-=-=-=-
// public - ctor
    network_object::network_object(
        const rcComm_t& _comm ) :
        socket_handle_( _comm.sock ) {

    } // ctor

// =-=-=-=-=-=-=-
// public - ctor
    network_object::network_object(
        const rsComm_t& _comm ) :
        socket_handle_( _comm.sock ) {

    } // ctor

// =-=-=-=-=-=-=-
// public - cctor
    network_object::network_object(
        const network_object& _rhs ) :
        first_class_object( _rhs ) {
        socket_handle_ = _rhs.socket_handle_;

    } // cctor

// =-=-=-=-=-=-=-
// public - dtor
    network_object::~network_object() {
    } // dtor

// =-=-=-=-=-=-=-
// public - assignment operator
    network_object& network_object::operator=(
        const network_object& _rhs ) {
        socket_handle_ = _rhs.socket_handle_;
        return *this;

    } // operator=

// =-=-=-=-=-=-=-
// public - equivalence operator
    bool network_object::operator==(
        const network_object& _rhs ) const {
        return socket_handle_ == _rhs.socket_handle_;

    } // operator==

// =-=-=-=-=-=-=-
// public - get rule engine kvp
    error network_object::get_re_vars(
        rule_engine_vars_t& _kvp ) {

        std::stringstream ss;
        ss << socket_handle_;
        _kvp[SOCKET_HANDLE_KW] = ss.str().c_str();

        return SUCCESS();
    } // get_re_vars

// =-=-=-=-=-=-=-
// convertion to client comm ptr
    error network_object::to_client( rcComm_t* _comm ) {
        if ( !_comm ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "null comm ptr" );
        }

        _comm->sock = socket_handle_;

        return SUCCESS();

    } // to_client

// =-=-=-=-=-=-=-
// convertion to client comm ptr
    error network_object::to_server( rsComm_t* _comm ) {
        if ( !_comm ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "null comm ptr" );
        }

        _comm->sock = socket_handle_;

        return SUCCESS();

    } // to_server


}; // namespace irods



