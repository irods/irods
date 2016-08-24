#include "rcMisc.h"
#include "get_hier_from_leaf_id.h"
#include "irods_resource_manager.hpp"
#include "rsGetHierFromLeafId.hpp"

extern irods::resource_manager resc_mgr;

int rsGetHierFromLeafId(
    rsComm_t*        _comm,
    get_hier_inp_t*  _inp,
    get_hier_out_t** _out ) {
    if( !_comm || !_inp || !_out ) {
       addRErrorMsg(
           &_comm->rError,
           STDOUT_STATUS,
           "null input param(s)");
       return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    ( *_out ) = ( get_hier_out_t* )malloc( sizeof( get_hier_out_t ) );

    std::string hier;
    irods::error ret = resc_mgr.leaf_id_to_hier(
                           _inp->resc_id_,
                           hier );
    if( !ret.ok() ) {
       addRErrorMsg(
           &_comm->rError,
           STDOUT_STATUS,
           ret.result().c_str() );
       return ret.code();
    }

    rstrcpy( (*_out)->hier_, hier.c_str(), MAX_NAME_LEN );

    return 0;
}
