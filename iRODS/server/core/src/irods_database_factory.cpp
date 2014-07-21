// =-=-=-=-=-=-=-
// irods includes
#include "irods_database_factory.hpp"
#include "irods_client_server_negotiation.hpp"

namespace irods {
// =-=-=-=-=-=-=-
// super basic free factory function to create a database object
// of a given type
irods::error database_factory(
    const std::string&          _type,
    irods::database_object_ptr& _ptr ) {
    // =-=-=-=-=-=-=-
    // param check
    if ( _type.empty() ) {
        return ERROR(
                   SYS_INVALID_INPUT_PARAM,
                   "empty type string" );
    }

    // =-=-=-=-=-=-=-
    // given the incoming type request, create the given database object
    if ( irods::POSTGRES_DATABASE_PLUGIN == _type ) {
        irods::postgres_object* pgsql = new irods::postgres_object;
        if ( !pgsql ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "postgresql allocation failed" );
        }

        irods::database_object* dobj = dynamic_cast< irods::database_object* >( pgsql );
        if ( !dobj ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "postgresql dynamic cast failed" );
        }

        _ptr.reset( dobj );

    }
    else if ( irods::MYSQL_DATABASE_PLUGIN == _type ) {
        irods::mysql_object* mysql = new irods::mysql_object;
        if ( !mysql ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "mysql allocation failed" );
        }

        irods::database_object* dobj = dynamic_cast< irods::database_object* >( mysql );
        if ( !dobj ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "mysql dynamic cast failed" );
        }

        _ptr.reset( dobj );

    }
    else if ( irods::ORACLE_DATABASE_PLUGIN == _type ) {
        irods::oracle_object* oracle = new irods::oracle_object;
        if ( !oracle ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "oracle allocation failed" );
        }

        irods::database_object* dobj = dynamic_cast< irods::database_object* >( oracle );
        if ( !dobj ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "oracle dynamic cast failed" );
        }

        _ptr.reset( dobj );

    }
    else {
        std::string msg( "database type not recognized [" );
        msg += _type;
        msg += "]";
        return ERROR(
                   SYS_INVALID_INPUT_PARAM,
                   msg );

    }

    return SUCCESS();

} // database_factory

}; // namespace irods



