/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef _create_write_replicator_H_
#define _create_write_replicator_H_

#include "eirods_error.h"
#include "eirods_oper_replicator.h"

namespace eirods {

/**
 * @brief Replicator for create/write operations
 */
    class create_write_replicator : public oper_replicator {
    public:
        /// @brief ctor. Note the resource is the root resource of the whole tree
        create_write_replicator(const std::string& _resource, const std::string& _child);
        virtual ~create_write_replicator(void);

        error replicate(resource_operation_context* _ctx, const child_list_t& _siblings, const object_oper& _object_oper);

    private:
        std::string resource_;
        std::string child_;
    };
}; // namespace eirods

#endif // _create_write_replicator_H_
