#ifndef _IRODS_REPL_REBALANCE_HPP_
#define _IRODS_REPL_REBALANCE_HPP_

#include "irods/irods_error.hpp"
#include "irods/irods_resource_manager.hpp"
#include "irods/icatHighLevelRoutines.hpp"

#include <vector>
#include <string>
#include <utility>

namespace irods {
    using leaf_bundle_t = resource_manager::leaf_bundle_t;

    // throws irods::exception
    void update_out_of_date_replicas(
        irods::plugin_context& _ctx,
        const std::vector<leaf_bundle_t>& _leaf_bundles,
        const int _batch_size,
        const std::string& _invocation_timestamp,
        const std::string& resource_name);

    // throws irods::exception
    void create_missing_replicas(
        irods::plugin_context& _ctx,
        const std::vector<leaf_bundle_t>& _leaf_bundles,
        const int _batch_size,
        const std::string& _invocation_timestamp,
        const std::string& resource_name);
}
#endif // _IRODS_REPL_REBALANCE_HPP_
