// =-=-=-=-=-=-=-
#include "irods/getHierarchyForResc.h"
#include "irods/procApiRequest.h"
#include "irods/apiNumber.h"

int rcGetHierarchyForResc(
    rcComm_t*                   _comm,
    getHierarchyForRescInp_t*   _resc,
    getHierarchyForRescOut_t**  _hier ) {
    int status = procApiRequest(
                     _comm,
                     GET_HIER_FOR_RESC_AN,
                     _resc,
                     NULL,
                     ( void ** )_hier,
                     NULL );
    return status;

} // rcGetHierarchyForResc



