// =-=-=-=-=-=-=-
// irods includes
#include "irods_database_object.hpp"
#include "irods_database_manager.hpp"

namespace irods {

    // =-=-=-=-=-=-=-
    // public - ctor
    database_object::database_object() {

    } // ctor

    // =-=-=-=-=-=-=-
    // public - cctor
    database_object::database_object(
        const database_object& _rhs ) {


    } // cctor

    // =-=-=-=-=-=-=-
    // public - dtor
    database_object::~database_object() {
    } // dtor

    // =-=-=-=-=-=-=-
    // public - assignment operator
    database_object& database_object::operator=(
        const database_object& _rhs ) {

        return *this;

    } // operator=

    // =-=-=-=-=-=-=-
    // public - equivalence operator
    bool database_object::operator==(
        const database_object& _rhs ) const {
        return false;

    } // operator==

    // =-=-=-=-=-=-=-
    // public - get rule engine kvp
    error database_object::get_re_vars(
        keyValPair_t& _kvp ) {

        //addKeyVal( &_kvp, SOCKET_HANDLE_KW, ss.str().c_str() );

        return SUCCESS();

    } // get_re_vars

}; // namespace irods



