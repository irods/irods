// =-=-=-=-=-=-=-
// irods includes
#include "irods_generic_database_object.hpp"
#include "irods_database_manager.hpp"

namespace irods {

// =-=-=-=-=-=-=-
// public - ctor
    generic_database_object::generic_database_object(const std::string &_type) : type_(_type) {

    } // ctor

// =-=-=-=-=-=-=-
// public - cctor
    generic_database_object::generic_database_object(
        const generic_database_object& _rhs ) :
        database_object( _rhs ), type_(_rhs.type_) {

    } // cctor

// =-=-=-=-=-=-=-
// public - dtor
    generic_database_object::~generic_database_object() {
    } // dtor

// =-=-=-=-=-=-=-
// public - assignment operator
    generic_database_object& generic_database_object::operator=(
        const generic_database_object& ) {

        return *this;

    } // operator=

// =-=-=-=-=-=-=-
// public - equivalence operator
    bool generic_database_object::operator==(
        const generic_database_object& ) const {
        return false;

    } // operator==

// =-=-=-=-=-=-=-
// plugin resolution operation
    error generic_database_object::resolve(
        const std::string& _interface,
        plugin_ptr&        _ptr ) {
        // =-=-=-=-=-=-=-
        // check the interface type and error out if it
        // isnt a database interface
        if ( DATABASE_INTERFACE != _interface ) {
            std::stringstream msg;
            msg << "generic_object does not support a [";
            msg << _interface;
            msg << "] plugin interface";
            return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );

        }

        // =-=-=-=-=-=-=-
        // ask the database manager for a generic resource
        database_ptr db_ptr;
        error ret = db_mgr.resolve( type_, db_ptr );
        if ( !ret.ok() ) {
            // =-=-=-=-=-=-=-
            // attempt to load the plugin, in this case the type,
            // instance name, key etc are all tcp as there is only
            // the need for one instance of a tcp object, etc.
            std::string empty_context( "" );
            ret = db_mgr.init_from_type(
                      type_,
                      type_,
                      type_,
                      empty_context,
                      db_ptr );
            if ( !ret.ok() ) {
                return PASS( ret );

            }
            else {
                // =-=-=-=-=-=-=-
                // upcast for out variable
                _ptr = boost::dynamic_pointer_cast< plugin_base >( db_ptr );
                return SUCCESS();

            }

        } // if !ok

        // =-=-=-=-=-=-=-
        // upcast for out variable
        _ptr = boost::dynamic_pointer_cast< plugin_base >( db_ptr );

        return SUCCESS();

    } // resolve

// =-=-=-=-=-=-=-
// public - get rule engine kvp
    error generic_database_object::get_re_vars(
        rule_engine_vars_t& ) {

        return SUCCESS();

    } // get_re_vars

}; // namespace irods



