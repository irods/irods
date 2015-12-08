#include "irods_unlink_replicator.hpp"

#include "dataObjUnlink.h"

namespace irods {

    unlink_replicator::unlink_replicator(
        const std::string& _child,
        const std::string& _resource ) {
        child_ = _child;
        resource_ = _resource;
    }

    unlink_replicator::~unlink_replicator( void ) {
        // TODO - stub
    }

    error unlink_replicator::replicate(
        plugin_context& _ctx,
        const child_list_t& _siblings,
        const object_oper& _object_oper ) {
        error result = SUCCESS();
        if ( ( result = ASSERT_ERROR( _object_oper.operation() == unlink_oper, INVALID_OPERATION,
                                      "Performing replication of unlink operation but specified operation is \"%s\".",
                                      _object_oper.operation().c_str() ) ).ok() ) {

            // Generate a sub hier up to and including this resource for the pdmo string
            hierarchy_parser sub_parser;
            sub_parser.set_string( child_ );
            std::string sub_hier;
            sub_parser.str( sub_hier, resource_ );

            file_object object = _object_oper.object();
            child_list_t::const_iterator it;
            for ( it = _siblings.begin(); result.ok() && it != _siblings.end(); ++it ) {
                hierarchy_parser sibling = *it;
                std::string hierarchy_string;
                error ret = sibling.str( hierarchy_string );
                if ( ( result = ASSERT_PASS( ret, "Failed to get the hierarchy string from the sibling hierarchy parser." ) ).ok() ) {

                    dataObjInp_t dataObjInp;
                    bzero( &dataObjInp, sizeof( dataObjInp ) );
                    rstrcpy( dataObjInp.objPath, object.logical_path().c_str(), MAX_NAME_LEN );
                    addKeyVal( &dataObjInp.condInput, RESC_HIER_STR_KW, hierarchy_string.c_str() );
                    addKeyVal( &dataObjInp.condInput, FORCE_FLAG_KW, "" );
                    addKeyVal( &dataObjInp.condInput, IN_PDMO_KW, sub_hier.c_str() );
                    int status = rsDataObjUnlink( _ctx.comm(), &dataObjInp );
                    // CAT_NO_ROWS_FOUND is okay if the file being unlinked is a registered file in which case there are no replicas
                    char* sys_error = NULL ;
                    const char* rods_error = rodsErrorName( status, &sys_error );
                    result = ASSERT_ERROR( status >= 0 || status == CAT_NO_ROWS_FOUND, status,
                                           "Failed to unlink the object: \"%s\" from the resource: \"%s\" - %s %s.",
                                           object.logical_path().c_str(), hierarchy_string.c_str(), rods_error, sys_error );
                    free( sys_error );
                }
            }
        }
        return result;
    }

}; // namespace irods
