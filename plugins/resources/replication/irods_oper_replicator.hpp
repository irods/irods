#ifndef _IRODS_OPER_REPLICATOR_HPP_
#define _IRODS_OPER_REPLICATOR_HPP_

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
            virtual error replicate( plugin_context& _ctx, const child_list_t& _siblings, const object_oper& _object_oper ) = 0;

            virtual ~oper_replicator() {

            }
    };
}; // namespace irods

#endif // _IRODS_OPER_REPLICATOR_HPP_
