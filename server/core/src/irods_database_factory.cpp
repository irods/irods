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

        // given the incoming type request, create the given database object
        if ( irods::POSTGRES_DATABASE_PLUGIN == _type ) {
            _ptr.reset( new irods::postgres_object );
        }
        else if ( irods::MYSQL_DATABASE_PLUGIN == _type ) {
            _ptr.reset( new irods::mysql_object );
        }
        else if ( irods::ORACLE_DATABASE_PLUGIN == _type ) {
            _ptr.reset( new irods::oracle_object );
        }
        else {
	    _ptr.reset( new irods::generic_database_object(_type) );

        }

        return SUCCESS();

    } // database_factory

}; // namespace irods
