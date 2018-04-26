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
        const database_object& _rhs ) = default; // cctor

// =-=-=-=-=-=-=-
// public - dtor
    database_object::~database_object() = default; // dtor

// =-=-=-=-=-=-=-
// public - assignment operator
    database_object& database_object::operator=(
        const database_object& ) {

        return *this;

    } // operator=

// =-=-=-=-=-=-=-
// public - equivalence operator
    bool database_object::operator==(
        const database_object& ) const {
        return false;

    } // operator==

// =-=-=-=-=-=-=-
// public - get rule engine kvp
    error database_object::get_re_vars(
        rule_engine_vars_t& ) {

        return SUCCESS();

    } // get_re_vars

}; // namespace irods



