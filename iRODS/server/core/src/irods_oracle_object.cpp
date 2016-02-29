// =-=-=-=-=-=-=-
// irods includes
#include "irods_oracle_object.hpp"
#include "irods_database_manager.hpp"

namespace irods {

// =-=-=-=-=-=-=-
// public - ctor
    oracle_object::oracle_object() {

    } // ctor

// =-=-=-=-=-=-=-
// public - cctor
    oracle_object::oracle_object(
        const oracle_object& _rhs ) :
        database_object( _rhs ) {


    } // cctor

// =-=-=-=-=-=-=-
// public - dtor
    oracle_object::~oracle_object() {
    } // dtor

// =-=-=-=-=-=-=-
// public - assignment operator
    oracle_object& oracle_object::operator=(
        const oracle_object& ) {

        return *this;

    } // operator=

// =-=-=-=-=-=-=-
// public - equivalence operator
    bool oracle_object::operator==(
        const oracle_object& ) const {
        return false;

    } // operator==

// =-=-=-=-=-=-=-
// plugin resolution operation
    error oracle_object::resolve(
        const std::string& _interface,
        plugin_ptr&        _ptr ) {
        // =-=-=-=-=-=-=-
        // check the interface type and error out if it
        // isnt a database interface
        if ( DATABASE_INTERFACE != _interface ) {
            std::stringstream msg;
            msg << "oracle_object does not support a [";
            msg << _interface;
            msg << "] plugin interface";
            return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );

        }

        // =-=-=-=-=-=-=-
        // ask the database manager for a oracle resource
        database_ptr db_ptr;
        error ret = db_mgr.resolve( ORACLE_DATABASE_PLUGIN, db_ptr );
        if ( !ret.ok() ) {
            // =-=-=-=-=-=-=-
            // attempt to load the plugin, in this case the type,
            // instance name, key etc are all tcp as there is only
            // the need for one instance of a tcp object, etc.
            std::string empty_context( "" );
            ret = db_mgr.init_from_type(
                      ORACLE_DATABASE_PLUGIN,
                      ORACLE_DATABASE_PLUGIN,
                      ORACLE_DATABASE_PLUGIN,
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
    error oracle_object::get_re_vars(
        rule_engine_vars_t& ) {

        //addKeyVal( &_kvp, SOCKET_HANDLE_KW, ss.str().c_str() );

        return SUCCESS();

    } // get_re_vars

}; // namespace irods



