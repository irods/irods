


#ifndef __IRODS_REPL_REBALANCE_HPP__
#define __IRODS_REPL_REBALANCE_HPP__

// =-=-=-=-=-=-=-
#include "irods_error.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "icatHighLevelRoutines.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <vector>
#include <string>
#include <utility>

namespace irods {
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
    irods::error proc_results_for_rebalance(
        rsComm_t*,                        // comm object
        const std::string&,               // parent resc name
        const std::string&,               // child resc name
        const dist_child_result_t& );     // query results

}; // namespace irods

#endif // __IRODS_REPL_REBALANCE_HPP__

