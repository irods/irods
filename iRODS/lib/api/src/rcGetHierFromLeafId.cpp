
#include "get_hier_from_leaf_id.h"
#include "procApiRequest.h"
#include "apiNumber.h"

int rcGetHierFromLeafId(
    rcComm_t*        _comm,
    get_hier_inp_t*  _inp,
    get_hier_out_t** _out) {
    return procApiRequest(
               _comm,
               GET_HIER_FROM_LEAF_ID_AN,
               _inp, NULL,
               (void**)_out, NULL);
}
