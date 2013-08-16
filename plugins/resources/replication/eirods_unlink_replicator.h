/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef _unlink_replicator_H_
#define _unlink_replicator_H_

#include "eirods_error.h"
#include "eirods_oper_replicator.h"

namespace eirods {

/**
 * @brief Class to replicate the unlink operation
 */
    class unlink_replicator : public oper_replicator {
    public:
        /// @brief Undocumented
        unlink_replicator(void);
        virtual ~unlink_replicator(void);

        error replicate(resource_plugin_context& _ctx, const child_list_t& _siblings, const object_oper& _object_oper);
    };
}; // namespace eirods

#endif // _unlink_replicator_H_
