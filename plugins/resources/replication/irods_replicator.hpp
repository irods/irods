#ifndef _replicator_H_
#define _replicator_H_

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
        error replicate( resource_plugin_context& _ctx, const child_list_t& _siblings, object_list_t& _opers );

    private:
        oper_replicator* oper_replicator_;
    };
}; // namespace irods

#endif // _replicator_H_
