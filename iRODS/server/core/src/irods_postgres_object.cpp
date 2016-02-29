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
        const postgres_object& _rhs ) :
        database_object( _rhs ) {

    } // cctor

// =-=-=-=-=-=-=-
// public - dtor
    postgres_object::~postgres_object() {
    } // dtor

// =-=-=-=-=-=-=-
// public - assignment operator
    postgres_object& postgres_object::operator=(
        const postgres_object& ) {

        return *this;

    } // operator=

// =-=-=-=-=-=-=-
// public - equivalence operator
    bool postgres_object::operator==(
        const postgres_object& ) const {
        return false;

    } // operator==

// =-=-=-=-=-=-=-
// plugin resolution operation
    error postgres_object::resolve(
        const std::string& _interface,
        plugin_ptr&        _ptr ) {
        // =-=-=-=-=-=-=-
        // check the interface type and error out if it
        // isnt a database interface
        if ( DATABASE_INTERFACE != _interface ) {
            std::stringstream msg;
            msg << "postgres_object does not support a [";
            msg << _interface;
            msg << "] plugin interface";
            return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );

        }

        // =-=-=-=-=-=-=-
        // ask the database manager for a postgres resource
        database_ptr db_ptr;
        error ret = db_mgr.resolve( POSTGRES_DATABASE_PLUGIN, db_ptr );
        if ( !ret.ok() ) {
            // =-=-=-=-=-=-=-
            // attempt to load the plugin, in this case the type,
            // instance name, key etc are all tcp as there is only
            // the need for one instance of a tcp object, etc.
            std::string empty_context( "" );
            ret = db_mgr.init_from_type(
                      POSTGRES_DATABASE_PLUGIN,
                      POSTGRES_DATABASE_PLUGIN,
                      POSTGRES_DATABASE_PLUGIN,
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
    error postgres_object::get_re_vars(
        rule_engine_vars_t& ) {

        return SUCCESS();

    } // get_re_vars

}; // namespace irods



