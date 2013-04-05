/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef _oper_replicator_H_
#define _oper_replicator_H_

#include "eirods_error.h"
#include "eirods_object_oper.h"
#include "eirods_repl_types.h"

#include <string>

namespace eirods {

/**
 * @brief Abstract base class for holding the implementation of the replication logic for different operations
 */
    class oper_replicator {
    public:
        virtual error replicate(rsComm_t* _comm, const child_list_t& _siblings, const object_oper& _object_oper) = 0;
    };
}; // namespace eirods

#endif // _oper_replicator_H_
