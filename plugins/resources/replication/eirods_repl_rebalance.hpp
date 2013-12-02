


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
    /// =-=-=-=-=-=-=-
    /// @brief gather a limit bound result set of all data objects 
    ///        which need re-replicated
    error gather_data_objects_for_rebalance( 
        rsComm_t*,              // comm object
        const std::string&,     // parent name
        const std::string&,     // child name
        const int,              // query limit
        dist_child_result_t& ); // result set
    /// =-=-=-=-=-=-=-
    /// @brief refresh a limit bound result set of all data objects 
    ///        which need re-replicated
    eirods::error proc_results_for_rebalance(
        rsComm_t*,                        // comm object
        const std::string&,               // parent resc name
        const std::string&,               // child resc name
        const dist_child_result_t& );     // query results

}; // namespace eirods

#endif // __EIRODS_REPL_REBALANCE_H__

