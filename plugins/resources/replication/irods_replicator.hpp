#ifndef _IRODS_REPLICATOR_HPP_
#define _IRODS_REPLICATOR_HPP_

#include "irods_error.hpp"
#include "irods_oper_replicator.hpp"
#include "irods_repl_types.hpp"

namespace irods {

    /**
     * @brief Class to replicate operations across all of a replicating nodes children
     */
    class replicator {
        public:
            /// @brief ctor
            replicator( oper_replicator* _oper_replicator );
            virtual ~replicator( void );

            /// @brief The function that replicates the specifed operations to the siblings
            error replicate( plugin_context& _ctx, const child_list_t& _siblings, object_list_t& _opers );

        private:
            oper_replicator* oper_replicator_;
    };
}; // namespace irods

#endif // _REPLICATOR_HPP_
