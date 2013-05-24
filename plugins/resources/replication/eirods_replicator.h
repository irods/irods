/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef _replicator_H_
#define _replicator_H_

#include "eirods_error.h"
#include "eirods_oper_replicator.h"
#include "eirods_repl_types.h"

namespace eirods {

/**
 * @brief Class to replicate operations across all of a replicating nodes children
 */
    class replicator {
    public:
        /// @brief ctor
        replicator(oper_replicator* _oper_replicator);
        virtual ~replicator(void);

        /// @brief The function that replicates the specifed operations to the siblings
        error replicate(resource_operation_context* _ctx, const child_list_t& _siblings, object_list_t& _opers);
        
    private:
        oper_replicator* oper_replicator_;
    };
}; // namespace eirods

#endif // _replicator_H_
