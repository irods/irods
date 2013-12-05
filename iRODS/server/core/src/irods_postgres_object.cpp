/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

// =-=-=-=-=-=-=-
// irods includes
#include "irods_postgres_object.hpp"
#include "irods_database_manager.hpp"

namespace irods {

    // =-=-=-=-=-=-=-
    // public - ctor
    postgres_object::postgres_object() {

    } // ctor

    // =-=-=-=-=-=-=-
    // public - cctor
    postgres_object::postgres_object(
        const postgres_object& _rhs ) {


    } // cctor

    // =-=-=-=-=-=-=-
    // public - dtor
    postgres_object::~postgres_object() {
    } // dtor

    // =-=-=-=-=-=-=-
    // public - assignment operator
    postgres_object& postgres_object::operator=(
        const postgres_object& _rhs ) {

        return *this;

    } // operator=

    // =-=-=-=-=-=-=-
    // public - equivalence operator
    bool postgres_object::operator==(
        const postgres_object& _rhs ) const {
        return false;

    } // operator==

    // =-=-=-=-=-=-=-
    // plugin resolution operation
    error postgres_object::resolve(
        const std::string& _interface,
        plugin_ptr&        _ptr ) {

        return SUCCESS();

    } // resolve

    // =-=-=-=-=-=-=-
    // public - get rule engine kvp
    error postgres_object::get_re_vars(
        keyValPair_t& _kvp ) {

        //addKeyVal( &_kvp, SOCKET_HANDLE_KW, ss.str().c_str() );

        return SUCCESS();

    } // get_re_vars

}; // namespace irods



