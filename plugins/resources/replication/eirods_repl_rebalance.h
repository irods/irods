


#ifndef __EIRODS_REPL_REBALANCE_H__
#define __EIRODS_REPL_REBALANCE_H__

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_error.h"

// =-=-=-=-=-=-=-
// irods includes
#include "icatHighLevelRoutines.h"

// =-=-=-=-=-=-=-
// stl includes
#include <vector>
#include <string>
#include <utility>

namespace eirods {
    eirods::error proc_result_for_rebalance(
        rsComm_t*,                        // comm object
        const std::string&,               // object path
        const std::string&,               // repl resc name
        const std::vector< std::string >&,// children
        const repl_obj_result_t& );       // query results

}; // namespace eirods

#endif // __EIRODS_REPL_REBALANCE_H__

