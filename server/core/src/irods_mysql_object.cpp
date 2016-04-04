// =-=-=-=-=-=-=-
// irods includes
#include "irods_mysql_object.hpp"
#include "irods_database_manager.hpp"

namespace irods {

// =-=-=-=-=-=-=-
// public - ctor
    mysql_object::mysql_object() {

    } // ctor

// =-=-=-=-=-=-=-
// public - cctor
    mysql_object::mysql_object(
        const mysql_object& _rhs ) :
        database_object( _rhs ) {

    } // cctor

// =-=-=-=-=-=-=-
// public - dtor
    mysql_object::~mysql_object() {
    } // dtor

// =-=-=-=-=-=-=-
// public - assignment operator
    mysql_object& mysql_object::operator=(
        const mysql_object& ) {

        return *this;

    } // operator=

// =-=-=-=-=-=-=-
// public - equivalence operator
    bool mysql_object::operator==(
        const mysql_object& ) const {
        return false;

    } // operator==

// =-=-=-=-=-=-=-
// plugin resolution operation
    error mysql_object::resolve(
        const std::string& _interface,
        plugin_ptr&        _ptr ) {
        // =-=-=-=-=-=-=-
        // check the interface type and error out if it
        // isnt a database interface
        if ( DATABASE_INTERFACE != _interface ) {
            std::stringstream msg;
            msg << "mysql_object does not support a [";
            msg << _interface;
            msg << "] plugin interface";
            return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );

        }

        // =-=-=-=-=-=-=-
        // ask the database manager for a mysql resource
        database_ptr db_ptr;
        error ret = db_mgr.resolve( MYSQL_DATABASE_PLUGIN, db_ptr );
        if ( !ret.ok() ) {
            // =-=-=-=-=-=-=-
            // attempt to load the plugin, in this case the type,
            // instance name, key etc are all tcp as there is only
            // the need for one instance of a tcp object, etc.
            std::string empty_context( "" );
            ret = db_mgr.init_from_type(
                      MYSQL_DATABASE_PLUGIN,
                      MYSQL_DATABASE_PLUGIN,
                      MYSQL_DATABASE_PLUGIN,
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
    error mysql_object::get_re_vars(
        rule_engine_vars_t& ) {

        return SUCCESS();

    } // get_re_vars

}; // namespace irods



