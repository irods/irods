#ifndef _IRODS_REPL_REBALANCE_HPP_
#define _IRODS_REPL_REBALANCE_HPP_

#include "irods_error.hpp"
#include "irods_resource_manager.hpp"
#include "icatHighLevelRoutines.hpp"

#include <vector>
#include <string>
#include <utility>

namespace irods {
    using leaf_bundle_t = resource_manager::leaf_bundle_t;
/// =-=-=-=-=-=-=-
/// @brief gather a limit bound result set of all data objects
///        which need re-replicated
    error gather_data_objects_for_rebalance(
        rsComm_t*,                   // comm object
        const int,                   // query limit
        size_t,                      // current bundle index
        std::vector<leaf_bundle_t>&, // all leaf bundles
        dist_child_result_t& );      // result set
/// =-=-=-=-=-=-=-
/// @brief refresh a limit bound result set of all data objects
///        which need re-replicated
    irods::error proc_results_for_rebalance(
        rsComm_t*,                        // comm object
        const std::string&,               // parent resc name
        const std::string&,               // child resc name
        const size_t,                     // current bundle index
        const std::vector<leaf_bundle_t>, // all leaf bundles
        const dist_child_result_t& );     // query results

}; // namespace irods

#endif // _IRODS_REPL_REBALANCE_HPP_

