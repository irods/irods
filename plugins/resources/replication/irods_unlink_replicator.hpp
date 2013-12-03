/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef _unlink_replicator_H_
#define _unlink_replicator_H_

#include "irods_error.hpp"
#include "irods_oper_replicator.hpp"

namespace irods {

/**
 * @brief Class to replicate the unlink operation
 */
    class unlink_replicator : public oper_replicator {
    public:
        /// @brief Constructs an unlink replicator with the specified unlinked child and with the specified resource.
        unlink_replicator(const std::string& _child, const std::string& _resource);
        virtual ~unlink_replicator(void);

        error replicate(resource_plugin_context& _ctx, const child_list_t& _siblings, const object_oper& _object_oper);
    private:
        std::string child_;
        std::string resource_;
    };
}; // namespace irods

#endif // _unlink_replicator_H_
