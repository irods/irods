/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef _oper_replicator_H_
#define _oper_replicator_H_

#include "irods_error.hpp"
#include "irods_object_oper.hpp"
#include "irods_repl_types.hpp"

#include <string>

namespace irods {

    /**
     * @brief Abstract base class for holding the implementation of the replication logic for different operations
     */
    class oper_replicator {
    public:
        virtual error replicate( resource_plugin_context& _ctx, const child_list_t& _siblings, const object_oper& _object_oper ) = 0;
    };
}; // namespace irods

#endif // _oper_replicator_H_
