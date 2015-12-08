#ifndef _IRODS_UNLINK_REPLICATOR_HPP_
#define _IRODS_UNLINK_REPLICATOR_HPP_

#include "irods_error.hpp"
#include "irods_oper_replicator.hpp"

namespace irods {

    /**
     * @brief Class to replicate the unlink operation
     */
    class unlink_replicator : public oper_replicator {
        public:
            /// @brief Constructs an unlink replicator with the specified unlinked child and with the specified resource.
            unlink_replicator( const std::string& _child, const std::string& _resource );
            virtual ~unlink_replicator( void );

            error replicate( plugin_context& _ctx, const child_list_t& _siblings, const object_oper& _object_oper );
        private:
            std::string child_;
            std::string resource_;
    };
}; // namespace irods

#endif // _IRODS_UNLINK_REPLICATOR_HPP_
