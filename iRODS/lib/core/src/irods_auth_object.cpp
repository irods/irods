/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */


#include "irods_auth_object.hpp"

namespace irods {

    auth_object::auth_object(
        rError_t* _r_error ) : r_error_( _r_error ) {
        // TODO - stub
    }

    auth_object::~auth_object() {
        // TODO - stub
    }

    auth_object::auth_object(
        const auth_object& _rhs ) {
        r_error_        = _rhs.r_error();
        request_result_ = _rhs.request_result();
        context_        = _rhs.context();
    }

    auth_object& auth_object::operator=(
        const auth_object& _rhs ) {
        r_error_        = _rhs.r_error();
        request_result_ = _rhs.request_result();
        context_        = _rhs.context();
        return *this;
    }

    bool auth_object::operator==(
        const auth_object& _rhs ) const {
        // For the base class just always return true
        return ( r_error_        == _rhs.r_error()        &&
                 request_result_ == _rhs.request_result() &&
                 context_        == _rhs.context() );
    }

}; // namespace irods
