/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef _create_write_replicator_H_
#define _create_write_replicator_H_

#include "eirods_error.hpp"
#include "eirods_oper_replicator.hpp"

namespace eirods {

/**
 * @brief Replicator for create/write operations
 */
    class create_write_replicator : public oper_replicator {
    public:
        /// @brief ctor. Note the resource is the root resource of the whole tree
        create_write_replicator(
            const std::string& _root_resource,    // The name of the resource at the root of the hierarchy
            const std::string& _current_resource, // The name of the resource at this level of hierarchy
            const std::string& _child);           // The hierarchy of the child.
        virtual ~create_write_replicator(void);

        error replicate(resource_plugin_context& _ctx, const child_list_t& _siblings, const object_oper& _object_oper);

    private:
        std::string root_resource_;
        std::string current_resource_;
        std::string child_;
    };
}; // namespace eirods

#endif // _create_write_replicator_H_
