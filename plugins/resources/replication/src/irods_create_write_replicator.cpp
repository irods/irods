#include "irods/private/irods_create_write_replicator.hpp"

#include "irods/dataObjRepl.h"
#include "irods/private/irods_repl_retry.hpp"
#include "irods/irods_stacktrace.hpp"

#include "fmt/format.h"

#include <cstring>

namespace irods {

    create_write_replicator::create_write_replicator(
        const std::string& _root_resource,
        const std::string& _current_resource,
        const std::string& _child )
      : root_resource_{_root_resource},
        current_resource_{_current_resource},
        child_{_child}{
    }

    create_write_replicator::~create_write_replicator( void ) {
        // TODO - stub
    }

    error create_write_replicator::replicate(
        plugin_context& _ctx,
        const child_list_t& _siblings,
        const object_oper& _object_oper ) {

        error result = SUCCESS();
        error last_error = SUCCESS();
        if ( ( result = ASSERT_ERROR( _object_oper.operation() == irods::CREATE_OPERATION || _object_oper.operation() == irods::WRITE_OPERATION,
                                      INVALID_OPERATION, "Performing create/write replication but objects operation is: \"%s\".",
                                      _object_oper.operation().c_str() ) ).ok() ) {
            // Generate a resource hierarchy string up to and including this resource
            hierarchy_parser child_parser;
            child_parser.set_string( child_ );
            std::string sub_hier;
            child_parser.str( sub_hier, current_resource_ );

            file_object object = _object_oper.object();
            child_list_t::const_iterator it;
            for ( it = _siblings.begin(); it != _siblings.end(); ++it ) {
                hierarchy_parser sibling = *it;
                std::string hierarchy_string;
                error ret = sibling.str( hierarchy_string );
                if ( ( result = ASSERT_PASS( ret, "Failed to get the hierarchy string from the sibling hierarchy parser." ) ).ok() ) {
                    dataObjInp_t dataObjInp;
                    std::memset(&dataObjInp, 0, sizeof(dataObjInp));
                    rstrcpy( dataObjInp.objPath, object.logical_path().c_str(), MAX_NAME_LEN );
                    dataObjInp.createMode = object.mode();

                    copyKeyVal( ( keyValPair_t* )&object.cond_input(), &dataObjInp.condInput );
                    addKeyVal( &dataObjInp.condInput, RESC_HIER_STR_KW, child_.c_str() );
                    addKeyVal( &dataObjInp.condInput, DEST_RESC_HIER_STR_KW, hierarchy_string.c_str() );
                    addKeyVal( &dataObjInp.condInput, RESC_NAME_KW, root_resource_.c_str() );
                    addKeyVal( &dataObjInp.condInput, DEST_RESC_NAME_KW, root_resource_.c_str() );
                    addKeyVal( &dataObjInp.condInput, IN_PDMO_KW, sub_hier.c_str() );

                    try {
                        const auto status = data_obj_repl_with_retry( _ctx, dataObjInp );
                        if (status < 0) {
                            // Replications which failed simply because they were not allowed need not be reported.
                            // Such failures should be fixed with a rebalance or some tree surgery.
                            if (SYS_NOT_ALLOWED == status) {
                                continue;
                            }

                            char* sys_error = NULL;
                            auto rods_error = rodsErrorName( status, &sys_error );
                            result = ERROR(status, fmt::format(
                                "Failed to replicate the data object: \"{}\" from resource: \"{}\" to sibling: \"{}\" - {} {}.",
                                object.logical_path(), child_, hierarchy_string, rods_error, sys_error));
                            free( sys_error );

                            // cache last error to return, log it and add it to the
                            // client side error stack
                            last_error = result;
                            irods::log( result );
                            addRErrorMsg(
                                &_ctx.comm()->rError,
                                result.code(),
                                result.result().c_str() );
                            result = SUCCESS();
                        }
                    }
                    catch ( const irods::exception& e ) {
                        irods::log( irods::error( e ) );
                    }

                } // if hier str

            } // for it

        } // if ok

        if ( !last_error.ok() ) {
            return last_error;
        }

        return SUCCESS();
    }

}; // namespace irods
